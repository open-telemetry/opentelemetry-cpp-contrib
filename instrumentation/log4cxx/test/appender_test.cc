/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <opentelemetry/instrumentation/log4cxx/appender.h>

#include <opentelemetry/logs/logger.h>
#include <opentelemetry/logs/logger_provider.h>
#include <opentelemetry/logs/provider.h>

#include <opentelemetry/exporters/ostream/log_record_exporter_factory.h>
#include <opentelemetry/sdk/logs/exporter.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/processor.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>

#include <log4cxx/logger.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace common    = opentelemetry::common;
namespace nostd     = opentelemetry::nostd;
namespace logs_api  = opentelemetry::logs;
namespace trace_api = opentelemetry::trace;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;

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

class OpenTelemetryAppenderTest : public testing::Test
{
public:
  log4cxx::LoggerPtr logger_;

protected:
  void SetUp() override
  {
    logger_ = log4cxx::Logger::getLogger("OTelLogger");
    logger_->addAppender(std::make_shared<log4cxx::OpenTelemetryAppender>());
  }

  void TearDown() override { logger_->removeAllAppenders(); }
};

TEST_F(OpenTelemetryAppenderTest, LevelToSeverity)
{
  using log4cxx::Level;
  using logs_api::Severity;
  using ota = log4cxx::OpenTelemetryAppender;

  ASSERT_TRUE(Severity::kFatal == ota::levelToSeverity(Level::FATAL_INT));
  ASSERT_TRUE(Severity::kError == ota::levelToSeverity(Level::ERROR_INT));
  ASSERT_TRUE(Severity::kWarn == ota::levelToSeverity(Level::WARN_INT));
  ASSERT_TRUE(Severity::kInfo == ota::levelToSeverity(Level::INFO_INT));
  ASSERT_TRUE(Severity::kDebug == ota::levelToSeverity(Level::DEBUG_INT));
  ASSERT_TRUE(Severity::kTrace == ota::levelToSeverity(Level::TRACE_INT));
  ASSERT_TRUE(Severity::kTrace == ota::levelToSeverity(Level::ALL_INT));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverity(Level::OFF_INT));
  ASSERT_TRUE(Severity::kTrace == ota::levelToSeverity(std::numeric_limits<int>::lowest()));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverity(std::numeric_limits<int>::lowest() + 1));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverity(-42));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverity(0));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverity(42));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverity(std::numeric_limits<int>::max() - 1));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverity(std::numeric_limits<int>::max()));
}

TEST_F(OpenTelemetryAppenderTest, Log_Success)
{
  auto provider_mock = new LoggerProviderMock();
  logs_api::Provider::SetLoggerProvider(nostd::shared_ptr<logs_api::LoggerProvider>(provider_mock));
  auto logger_mock    = new LoggerMock();
  auto logger_ptr     = nostd::shared_ptr<logs_api::Logger>(logger_mock);
  auto logrecord_mock = new LogRecordMock();
  auto logrecord_ptr  = nostd::unique_ptr<logs_api::LogRecord>(logrecord_mock);

  nostd::string_view logger_name;
  nostd::string_view library_name;
  nostd::string_view library_version;
  logs_api::Severity severity = {};
  common::AttributeValue message;
  common::SystemTimestamp timestamp;
  common::AttributeValue thread_name;

  auto pre_log = std::chrono::system_clock::now();
  EXPECT_CALL(*provider_mock, GetLogger(_, _, _, _, _))
      .WillOnce(DoAll(SaveArg<0>(&logger_name), SaveArg<1>(&library_name),
                      SaveArg<2>(&library_version), Return(logger_ptr)));
  EXPECT_CALL(*logger_mock, CreateLogRecord()).WillOnce(Return(std::move(logrecord_ptr)));
  EXPECT_CALL(*logrecord_mock, SetSeverity(_)).WillOnce(SaveArg<0>(&severity));
  EXPECT_CALL(*logrecord_mock, SetBody(_)).WillOnce(SaveArg<0>(&message));
  EXPECT_CALL(*logrecord_mock, SetTimestamp(_)).WillOnce(SaveArg<0>(&timestamp));
  EXPECT_CALL(*logrecord_mock, SetAttribute(nostd::string_view("thread.name"), _))
      .WillOnce(SaveArg<1>(&thread_name));
  EXPECT_CALL(*logger_mock, EmitLogRecord(_)).Times(1);

  LOG4CXX_INFO(logger_, "test message");
  auto post_log = std::chrono::system_clock::now();
  ASSERT_EQ(logger_name, "OTelLogger");
  ASSERT_EQ(library_name, "log4cxx");
  ASSERT_EQ(library_version, log4cxx::OpenTelemetryAppender::libraryVersion());
  ASSERT_TRUE(nostd::holds_alternative<nostd::string_view>(message));
  ASSERT_EQ(nostd::get<nostd::string_view>(message), "test message");
  ASSERT_TRUE(severity == logs_api::Severity::kInfo);
  ASSERT_TRUE(timestamp.time_since_epoch() >= pre_log.time_since_epoch());
  ASSERT_TRUE(timestamp.time_since_epoch() <= post_log.time_since_epoch());
  ASSERT_TRUE(nostd::holds_alternative<nostd::string_view>(thread_name));
  ASSERT_FALSE(nostd::get<nostd::string_view>(thread_name).empty());
}

TEST_F(OpenTelemetryAppenderTest, Log_Failure)
{
  auto provider_mock = new LoggerProviderMock();
  logs_api::Provider::SetLoggerProvider(nostd::shared_ptr<logs_api::LoggerProvider>(provider_mock));
  auto logger_mock    = new LoggerMock();
  auto logger_ptr     = nostd::shared_ptr<logs_api::Logger>(logger_mock);
  auto logrecord_mock = new LogRecordMock();
  auto logrecord_ptr  = nostd::unique_ptr<logs_api::LogRecord>(logrecord_mock);

  nostd::string_view logger_name;
  nostd::string_view library_name;
  nostd::string_view library_version;

  EXPECT_CALL(*provider_mock, GetLogger(_, _, _, _, _))
      .WillOnce(DoAll(SaveArg<0>(&logger_name), SaveArg<1>(&library_name),
                      SaveArg<2>(&library_version), Return(logger_ptr)));
  EXPECT_CALL(*logger_mock, CreateLogRecord()).WillOnce(Return(nullptr));
  EXPECT_CALL(*logger_mock, EmitLogRecord(_)).Times(0);

  LOG4CXX_INFO(logger_, "test message");
  ASSERT_EQ(logger_name, "OTelLogger");
  ASSERT_EQ(library_name, "log4cxx");
  ASSERT_EQ(library_version, log4cxx::OpenTelemetryAppender::libraryVersion());
}

TEST_F(OpenTelemetryAppenderTest, Multi_Threaded)
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

  const auto pre_log = std::chrono::system_clock::now().time_since_epoch().count();

  for (size_t index = 0; index < count; ++index)
  {
    threads.emplace_back([this, index]() { LOG4CXX_INFO(logger_, "Test message " << index); });
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
    ASSERT_TRUE(entry != messages.end());

    const auto offset = std::distance(messages.begin(), entry);
    ASSERT_GE(timestamps[offset], pre_log);
    ASSERT_LE(timestamps[offset], post_log);
  }
}
