/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <opentelemetry/instrumentation/boost_log/sink.h>

#include <opentelemetry/logs/logger.h>
#include <opentelemetry/logs/logger_provider.h>
#include <opentelemetry/logs/provider.h>

#include <opentelemetry/exporters/ostream/log_record_exporter_factory.h>
#include <opentelemetry/sdk/logs/exporter.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/processor.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>

#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace common    = opentelemetry::common;
namespace instr     = opentelemetry::instrumentation;
namespace nostd     = opentelemetry::nostd;
namespace logs_api  = opentelemetry::logs;
namespace trace_api = opentelemetry::trace;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;

struct LogRecordMock final : public logs_api::LogRecord
{
  MOCK_METHOD(void, SetTimestamp, (common::SystemTimestamp), (noexcept, override));
  MOCK_METHOD(void, SetObservedTimestamp, (common::SystemTimestamp), (noexcept, override));
  MOCK_METHOD(void, SetSeverity, (logs_api::Severity), (noexcept, override));
  MOCK_METHOD(void, SetBody, (const common::AttributeValue &), (noexcept, override));
  MOCK_METHOD(void,
              SetAttribute,
              (nostd::string_view, const common::AttributeValue &),
              (noexcept, override));
  MOCK_METHOD(void, SetEventId, (int64_t, nostd::string_view), (noexcept, override));
  MOCK_METHOD(void, SetTraceId, (const trace_api::TraceId &), (noexcept, override));
  MOCK_METHOD(void, SetSpanId, (const trace_api::SpanId &), (noexcept, override));
  MOCK_METHOD(void, SetTraceFlags, (const trace_api::TraceFlags &), (noexcept, override));
};

struct LoggerMock final : public logs_api::Logger
{
  MOCK_METHOD(const nostd::string_view, GetName, (), (noexcept, override));
  MOCK_METHOD(nostd::unique_ptr<logs_api::LogRecord>, CreateLogRecord, (), (noexcept, override));
  MOCK_METHOD(void,
              EmitLogRecord,
              (nostd::unique_ptr<logs_api::LogRecord> &&),
              (noexcept, override));
};

struct LoggerProviderMock final : public logs_api::LoggerProvider
{
  MOCK_METHOD(nostd::shared_ptr<logs_api::Logger>,
              GetLogger,
              (nostd::string_view,
               nostd::string_view,
               nostd::string_view,
               nostd::string_view,
               const common::KeyValueIterable &),
              (override));
};

class OpenTelemetrySinkTest : public testing::Test
{
protected:
  void SetUp() override
  {
    using boost::log::sinks::synchronous_sink;
    using opentelemetry::instrumentation::boost_log::OpenTelemetrySinkBackend;
    auto backend = boost::make_shared<OpenTelemetrySinkBackend>();
    auto sink    = boost::make_shared<synchronous_sink<OpenTelemetrySinkBackend>>(backend);
    boost::log::core::get()->add_sink(sink);
    boost::log::add_common_attributes();
  }

  void TearDown() override { boost::log::core::get()->remove_all_sinks(); }
};

TEST_F(OpenTelemetrySinkTest, LevelToSeverity)
{
  using Level = boost::log::trivial::severity_level;
  using logs_api::Severity;
  using ots          = instr::boost_log::OpenTelemetrySinkBackend;
  const auto lowest  = std::numeric_limits<int>::lowest();
  const auto highest = std::numeric_limits<int>::max();

  ASSERT_TRUE(Severity::kFatal == ots::levelToSeverity(Level::fatal));
  ASSERT_TRUE(Severity::kError == ots::levelToSeverity(Level::error));
  ASSERT_TRUE(Severity::kWarn == ots::levelToSeverity(Level::warning));
  ASSERT_TRUE(Severity::kInfo == ots::levelToSeverity(Level::info));
  ASSERT_TRUE(Severity::kDebug == ots::levelToSeverity(Level::debug));
  ASSERT_TRUE(Severity::kTrace == ots::levelToSeverity(Level::trace));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverity(static_cast<Level>(lowest)));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverity(static_cast<Level>(lowest + 1)));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverity(static_cast<Level>(-42)));
  ASSERT_TRUE(Severity::kTrace == ots::levelToSeverity(static_cast<Level>(0)));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverity(static_cast<Level>(42)));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverity(static_cast<Level>(highest - 1)));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverity(static_cast<Level>(highest)));
}

TEST_F(OpenTelemetrySinkTest, Log_Success)
{
  auto provider_mock = new LoggerProviderMock();
  logs_api::Provider::SetLoggerProvider(nostd::shared_ptr<logs_api::LoggerProvider>(provider_mock));
  auto logger_mock    = new LoggerMock();
  auto logger_ptr     = nostd::shared_ptr<logs_api::Logger>(logger_mock);
  auto logrecord_mock = new LogRecordMock();
  auto logrecord_ptr  = nostd::unique_ptr<logs_api::LogRecord>(logrecord_mock);

  boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger;
  auto pre_log = std::chrono::system_clock::now();

  EXPECT_CALL(*provider_mock, GetLogger(_, _, _, _, _))
      .WillOnce(DoAll(Invoke([](nostd::string_view logger_name, nostd::string_view library_name,
                                nostd::string_view library_version, nostd::string_view,
                                const common::KeyValueIterable &) {
                        ASSERT_EQ(logger_name, "Boost logger");
                        ASSERT_EQ(library_name, "Boost.Log");
                        ASSERT_EQ(library_version,
                                  instr::boost_log::OpenTelemetrySinkBackend::libraryVersion());
                      }),
                      Return(logger_ptr)));
  EXPECT_CALL(*logger_mock, CreateLogRecord()).WillOnce(Return(std::move(logrecord_ptr)));
  EXPECT_CALL(*logrecord_mock, SetSeverity(_)).WillOnce(Invoke([](logs_api::Severity severity) {
    ASSERT_TRUE(severity == logs_api::Severity::kInfo);
  }));
  EXPECT_CALL(*logrecord_mock, SetBody(_))
      .WillOnce(Invoke([](const common::AttributeValue &message) {
        ASSERT_TRUE(nostd::holds_alternative<nostd::string_view>(message));
        ASSERT_EQ(nostd::get<nostd::string_view>(message), "test message");
      }));
  EXPECT_CALL(*logrecord_mock, SetTimestamp(_))
      .WillOnce(Invoke([pre_log](common::SystemTimestamp timestamp) {
        auto post_log = std::chrono::system_clock::now();
        ASSERT_TRUE(timestamp.time_since_epoch() >= pre_log.time_since_epoch());
        ASSERT_TRUE(timestamp.time_since_epoch() <= post_log.time_since_epoch());
      }));
  EXPECT_CALL(*logrecord_mock, SetAttribute(nostd::string_view("code.lineno"), _))
      .WillOnce(Invoke([](nostd::string_view, const common::AttributeValue &line_number) {
        ASSERT_TRUE(nostd::holds_alternative<int>(line_number));
        ASSERT_GE(nostd::get<int>(line_number), 0);
      }));
  EXPECT_CALL(*logrecord_mock, SetAttribute(nostd::string_view("code.filepath"), _))
      .WillOnce(Invoke([](nostd::string_view, const common::AttributeValue &file_name) {
        ASSERT_TRUE(nostd::holds_alternative<nostd::string_view>(file_name));
        ASSERT_TRUE(std::string(nostd::get<nostd::string_view>(file_name)).find("sink_test.cc") !=
                    std::string::npos);
      }));
  EXPECT_CALL(*logrecord_mock, SetAttribute(nostd::string_view("code.function"), _))
      .WillOnce(Invoke([](nostd::string_view, const common::AttributeValue &func_name) {
        ASSERT_TRUE(nostd::holds_alternative<nostd::string_view>(func_name));
        ASSERT_TRUE(nostd::get<nostd::string_view>(func_name) == "TestBody");
      }));
  EXPECT_CALL(*logrecord_mock, SetAttribute(nostd::string_view("thread.id"), _))
      .WillOnce(Invoke([](nostd::string_view, const common::AttributeValue &thread_id) {
        ASSERT_TRUE(nostd::holds_alternative<nostd::string_view>(thread_id));
        ASSERT_FALSE(nostd::get<nostd::string_view>(thread_id).empty());
      }));
  EXPECT_CALL(*logger_mock, EmitLogRecord(_)).Times(1);

  BOOST_LOG_SEV(logger, boost::log::trivial::info)
      << boost::log::add_value("FileName", __FILE__)
      << boost::log::add_value("FunctionName", __FUNCTION__)
      << boost::log::add_value("LineNumber", __LINE__) << "test message";
}

TEST_F(OpenTelemetrySinkTest, Log_Failure)
{
  auto provider_mock = new LoggerProviderMock();
  logs_api::Provider::SetLoggerProvider(nostd::shared_ptr<logs_api::LoggerProvider>(provider_mock));
  auto logger_mock = new LoggerMock();
  auto logger_ptr  = nostd::shared_ptr<logs_api::Logger>(logger_mock);
  boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger;

  EXPECT_CALL(*provider_mock, GetLogger(_, _, _, _, _))
      .WillOnce(DoAll(Invoke([](nostd::string_view logger_name, nostd::string_view library_name,
                                nostd::string_view library_version, nostd::string_view,
                                const common::KeyValueIterable &) {
                        ASSERT_EQ(logger_name, "Boost logger");
                        ASSERT_EQ(library_name, "Boost.Log");
                        ASSERT_EQ(library_version,
                                  instr::boost_log::OpenTelemetrySinkBackend::libraryVersion());
                      }),
                      Return(logger_ptr)));
  EXPECT_CALL(*logger_mock, CreateLogRecord()).WillOnce(Return(nullptr));
  EXPECT_CALL(*logger_mock, EmitLogRecord(_)).Times(0);

  BOOST_LOG_SEV(logger, boost::log::trivial::info) << "test message";
}

TEST_F(OpenTelemetrySinkTest, Log_WithoutSeverity)
{
  auto provider_mock = new LoggerProviderMock();
  logs_api::Provider::SetLoggerProvider(nostd::shared_ptr<logs_api::LoggerProvider>(provider_mock));
  auto logger_mock    = new LoggerMock();
  auto logger_ptr     = nostd::shared_ptr<logs_api::Logger>(logger_mock);
  auto logrecord_mock = new LogRecordMock();
  auto logrecord_ptr  = nostd::unique_ptr<logs_api::LogRecord>(logrecord_mock);

  boost::log::sources::logger logger;
  auto pre_log = std::chrono::system_clock::now();

  EXPECT_CALL(*provider_mock, GetLogger(_, _, _, _, _))
      .WillOnce(DoAll(Invoke([](nostd::string_view logger_name, nostd::string_view library_name,
                                nostd::string_view library_version, nostd::string_view,
                                const common::KeyValueIterable &) {
                        ASSERT_EQ(logger_name, "Boost logger");
                        ASSERT_EQ(library_name, "Boost.Log");
                        ASSERT_EQ(library_version,
                                  instr::boost_log::OpenTelemetrySinkBackend::libraryVersion());
                      }),
                      Return(logger_ptr)));
  EXPECT_CALL(*logger_mock, CreateLogRecord()).WillOnce(Return(std::move(logrecord_ptr)));
  EXPECT_CALL(*logrecord_mock, SetSeverity(_)).WillOnce(Invoke([](logs_api::Severity severity) {
    ASSERT_TRUE(severity == logs_api::Severity::kInvalid);
  }));
  EXPECT_CALL(*logrecord_mock, SetBody(_))
      .WillOnce(Invoke([](const common::AttributeValue &message) {
        ASSERT_TRUE(nostd::holds_alternative<nostd::string_view>(message));
        ASSERT_EQ(nostd::get<nostd::string_view>(message), "no severity");
      }));
  EXPECT_CALL(*logrecord_mock, SetTimestamp(_))
      .WillOnce(Invoke([pre_log](common::SystemTimestamp timestamp) {
        auto post_log = std::chrono::system_clock::now();
        ASSERT_TRUE(timestamp.time_since_epoch() >= pre_log.time_since_epoch());
        ASSERT_TRUE(timestamp.time_since_epoch() <= post_log.time_since_epoch());
      }));
  EXPECT_CALL(*logrecord_mock, SetAttribute(nostd::string_view("thread.id"), _)).Times(1);
  EXPECT_CALL(*logger_mock, EmitLogRecord(_)).Times(1);

  BOOST_LOG(logger) << "no severity";
}

TEST_F(OpenTelemetrySinkTest, Multi_Threaded)
{
  namespace logs_sdk = opentelemetry::sdk::logs;
  namespace logs_exp = opentelemetry::exporter::logs;

  // Set up logger provider
  auto exporter     = logs_exp::OStreamLogRecordExporterFactory::Create();
  auto processor    = logs_sdk::SimpleLogRecordProcessorFactory::Create(std::move(exporter));
  auto provider     = logs_sdk::LoggerProviderFactory::Create(std::move(processor));
  auto provider_ptr = nostd::shared_ptr<logs_api::LoggerProvider>(provider.release());
  logs_api::Provider::SetLoggerProvider(provider_ptr);
  // Save original stream buffer, then redirect cout to our new stream buffer
  std::streambuf *original = std::cout.rdbuf();
  std::stringstream output;
  std::cout.rdbuf(output.rdbuf());
  // Set up logging threads
  const auto count = 100UL;
  std::vector<std::thread> threads;
  threads.reserve(count);

  boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger;
  const auto pre_log = std::chrono::system_clock::now().time_since_epoch().count();

  for (size_t index = 0; index < count; ++index)
  {
    threads.emplace_back([&logger, index]() {
      BOOST_LOG_SEV(logger, boost::log::trivial::info) << "Test message " << index;
    });
  }

  for (auto &task : threads)
  {
    if (task.joinable())
    {
      task.join();
    }
  }

  const auto post_log = std::chrono::system_clock::now().time_since_epoch().count();

  // Reset cout's original stringstream buffer
  std::cout.rdbuf(original);
  // Extract messages with timestamps
  const auto field_name_length = 23UL;
  std::vector<std::string> messages;
  std::vector<uint64_t> timestamps;
  std::string str;

  while (std::getline(output, str, '\n'))
  {
    if (str.find(" timestamp          : ") != std::string::npos)
    {
      timestamps.push_back(std::strtoul(str.substr(field_name_length).c_str(), nullptr, 10));
    }
    else if (str.find(" body               : ") != std::string::npos)
    {
      messages.push_back(str.substr(field_name_length));
    }
  }

  for (size_t index = 0; index < count; ++index)
  {
    const auto &message = "Test message " + std::to_string(index);
    const auto &entry   = std::find(messages.begin(), messages.end(), message);
    ASSERT_TRUE(entry != messages.end()) << message;

    const auto offset = std::distance(messages.begin(), entry);
    ASSERT_GE(timestamps[offset], pre_log);
    ASSERT_LE(timestamps[offset], post_log);
  }
}

void SetUpBackendWithDummyMappers()
{
  opentelemetry::instrumentation::boost_log::ValueMappers mappers;
  mappers.ToSeverity = [](const boost::log::record_view &) {
    return opentelemetry::logs::Severity::kTrace4;
  };
  mappers.ToTimeStamp = [](const boost::log::record_view &,
                           std::chrono::system_clock::time_point &) {
    // This should prevent SetTimestamp attribute from being called entirely
    return false;
  };
  mappers.ToCodeLine = [](const boost::log::record_view &, int &code_line) {
    code_line = 42;
    return true;
  };
  mappers.ToCodeFunc = [](const boost::log::record_view &, std::string &func_name) {
    func_name = "doFoo";
    return true;
  };
  mappers.ToCodeFile = [](const boost::log::record_view &, std::string &file_name) {
    file_name = "bar.cpp";
    return true;
  };
  mappers.ToThreadId = [](const boost::log::record_view &, std::string &thread_id) {
    thread_id = "0x600df457c0d3";
    return true;
  };

  using boost::log::sinks::synchronous_sink;
  using opentelemetry::instrumentation::boost_log::OpenTelemetrySinkBackend;
  auto backend = boost::make_shared<OpenTelemetrySinkBackend>(mappers);
  auto sink    = boost::make_shared<synchronous_sink<OpenTelemetrySinkBackend>>(backend);
  boost::log::core::get()->add_sink(sink);
}

TEST(OpenTelemetrySinkTestSuite, CustomMappers)
{
  auto provider_mock = new LoggerProviderMock();
  logs_api::Provider::SetLoggerProvider(nostd::shared_ptr<logs_api::LoggerProvider>(provider_mock));
  auto logger_mock    = new LoggerMock();
  auto logger_ptr     = nostd::shared_ptr<logs_api::Logger>(logger_mock);
  auto logrecord_mock = new LogRecordMock();
  auto logrecord_ptr  = nostd::unique_ptr<logs_api::LogRecord>(logrecord_mock);

  SetUpBackendWithDummyMappers();
  boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger;

  EXPECT_CALL(*provider_mock, GetLogger(_, _, _, _, _))
      .WillOnce(DoAll(Invoke([](nostd::string_view logger_name, nostd::string_view library_name,
                                nostd::string_view library_version, nostd::string_view,
                                const common::KeyValueIterable &) {
                        ASSERT_EQ(logger_name, "Boost logger");
                        ASSERT_EQ(library_name, "Boost.Log");
                        ASSERT_EQ(library_version,
                                  instr::boost_log::OpenTelemetrySinkBackend::libraryVersion());
                      }),
                      Return(logger_ptr)));
  EXPECT_CALL(*logger_mock, CreateLogRecord()).WillOnce(Return(std::move(logrecord_ptr)));
  EXPECT_CALL(*logrecord_mock, SetSeverity(_)).WillOnce(Invoke([](logs_api::Severity severity) {
    ASSERT_TRUE(severity == logs_api::Severity::kTrace4);
  }));
  EXPECT_CALL(*logrecord_mock, SetBody(_))
      .WillOnce(Invoke([](const common::AttributeValue &message) {
        ASSERT_TRUE(nostd::holds_alternative<nostd::string_view>(message));
        ASSERT_EQ(nostd::get<nostd::string_view>(message), "custom mappers");
      }));
  EXPECT_CALL(*logrecord_mock, SetTimestamp(_)).Times(0);  // Intended - test returning false
  EXPECT_CALL(*logrecord_mock, SetAttribute(nostd::string_view("code.lineno"), _))
      .WillOnce(Invoke([](nostd::string_view, const common::AttributeValue &line_number) {
        ASSERT_TRUE(nostd::holds_alternative<int>(line_number));
        ASSERT_EQ(nostd::get<int>(line_number), 42);
      }));
  EXPECT_CALL(*logrecord_mock, SetAttribute(nostd::string_view("code.filepath"), _))
      .WillOnce(Invoke([](nostd::string_view, const common::AttributeValue &file_name) {
        ASSERT_TRUE(nostd::holds_alternative<nostd::string_view>(file_name));
        ASSERT_TRUE(nostd::get<nostd::string_view>(file_name) == "bar.cpp");
      }));
  EXPECT_CALL(*logrecord_mock, SetAttribute(nostd::string_view("code.function"), _))
      .WillOnce(Invoke([](nostd::string_view, const common::AttributeValue &func_name) {
        ASSERT_TRUE(nostd::holds_alternative<nostd::string_view>(func_name));
        ASSERT_TRUE(nostd::get<nostd::string_view>(func_name) == "doFoo");
      }));
  EXPECT_CALL(*logrecord_mock, SetAttribute(nostd::string_view("thread.id"), _))
      .WillOnce(Invoke([](nostd::string_view, const common::AttributeValue &thread_id) {
        ASSERT_TRUE(nostd::holds_alternative<nostd::string_view>(thread_id));
        ASSERT_TRUE(nostd::get<nostd::string_view>(thread_id) == "0x600df457c0d3");
      }));
  EXPECT_CALL(*logger_mock, EmitLogRecord(_)).Times(1);

  BOOST_LOG(logger) << "custom mappers";
  boost::log::core::get()->remove_all_sinks();
}

enum class CustomSeverity
{
  kRed,
  kOrange,
  kYellow,
  kGreen,
  kBlue,
  kIndigo,
  kViolet
};

class CustomSeverityTest
    : public ::testing::TestWithParam<std::tuple<CustomSeverity, opentelemetry::logs::Severity>>
{
protected:
  void SetUp() override
  {
    opentelemetry::instrumentation::boost_log::ValueMappers mappers;
    mappers.ToSeverity = [](const boost::log::record_view &record) {
      if (const auto &result = boost::log::extract<CustomSeverity>(record["Severity"]))
      {
        switch (result.get())
        {
          using opentelemetry::logs::Severity;

          case CustomSeverity::kRed:
            return Severity::kFatal;
          case CustomSeverity::kOrange:
            return Severity::kError;
          case CustomSeverity::kYellow:
            return Severity::kWarn;
          case CustomSeverity::kGreen:
            return Severity::kInfo;
          case CustomSeverity::kBlue:
            return Severity::kDebug;
          case CustomSeverity::kIndigo:
            return Severity::kTrace;
          default:
            return Severity::kInvalid;
        }
      }

      return opentelemetry::logs::Severity::kInvalid;
    };

    using boost::log::sinks::synchronous_sink;
    using opentelemetry::instrumentation::boost_log::OpenTelemetrySinkBackend;
    auto backend = boost::make_shared<OpenTelemetrySinkBackend>(mappers);
    auto sink    = boost::make_shared<synchronous_sink<OpenTelemetrySinkBackend>>(backend);
    boost::log::core::get()->add_sink(sink);
    boost::log::add_common_attributes();
  }

  void TearDown() override { boost::log::core::get()->remove_all_sinks(); }
};

TEST_P(CustomSeverityTest, LevelMapping)
{
  const auto input_level    = std::get<0>(GetParam());
  const auto expected_level = std::get<1>(GetParam());

  auto provider_mock = new LoggerProviderMock();
  logs_api::Provider::SetLoggerProvider(nostd::shared_ptr<logs_api::LoggerProvider>(provider_mock));
  auto logger_mock    = new LoggerMock();
  auto logger_ptr     = nostd::shared_ptr<logs_api::Logger>(logger_mock);
  auto logrecord_mock = new LogRecordMock();
  auto logrecord_ptr  = nostd::unique_ptr<logs_api::LogRecord>(logrecord_mock);
  boost::log::sources::severity_logger<CustomSeverity> logger;

  EXPECT_CALL(*provider_mock, GetLogger(_, _, _, _, _)).WillOnce(Return(logger_ptr));
  EXPECT_CALL(*logger_mock, CreateLogRecord()).WillOnce(Return(std::move(logrecord_ptr)));
  EXPECT_CALL(*logrecord_mock, SetSeverity(_))
      .WillOnce(Invoke([expected_level](logs_api::Severity severity) {
        ASSERT_TRUE(severity == expected_level);
      }));
  EXPECT_CALL(*logrecord_mock, SetTimestamp(_)).Times(1);
  EXPECT_CALL(*logrecord_mock, SetAttribute(nostd::string_view("thread.id"), _)).Times(1);
  EXPECT_CALL(*logrecord_mock, SetBody(_)).Times(1);
  EXPECT_CALL(*logger_mock, EmitLogRecord(_)).Times(1);

  BOOST_LOG_SEV(logger, input_level);
}

INSTANTIATE_TEST_SUITE_P(
    OpenTelemetrySinkTestSuite,
    CustomSeverityTest,
    ::testing::Values(
        std::make_tuple(CustomSeverity::kRed, opentelemetry::logs::Severity::kFatal),
        std::make_tuple(CustomSeverity::kOrange, opentelemetry::logs::Severity::kError),
        std::make_tuple(CustomSeverity::kYellow, opentelemetry::logs::Severity::kWarn),
        std::make_tuple(CustomSeverity::kGreen, opentelemetry::logs::Severity::kInfo),
        std::make_tuple(CustomSeverity::kBlue, opentelemetry::logs::Severity::kDebug),
        std::make_tuple(CustomSeverity::kIndigo, opentelemetry::logs::Severity::kTrace),
        std::make_tuple(CustomSeverity::kViolet, opentelemetry::logs::Severity::kInvalid)));
