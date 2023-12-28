/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <opentelemetry/instrumentation/spdlog/sink.h>

#include <opentelemetry/logs/logger.h>
#include <opentelemetry/logs/logger_provider.h>
#include <opentelemetry/logs/provider.h>

#include <opentelemetry/exporters/ostream/log_record_exporter_factory.h>
#include <opentelemetry/sdk/logs/exporter.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/processor.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>

#include <spdlog/spdlog.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace common    = opentelemetry::common;
namespace nostd     = opentelemetry::nostd;
namespace logs_api  = opentelemetry::logs;
namespace trace_api = opentelemetry::trace;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
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

class OpenTelemetrySinkTest : public testing::Test
{
public:
  nostd::shared_ptr<spdlog::logger> logger_;

protected:
  void SetUp() override
  {
    auto otel_sink = std::make_shared<spdlog::sinks::opentelemetry_sink_mt>();
    otel_sink->set_level(spdlog::level::info);
    logger_ = nostd::shared_ptr<spdlog::logger>(new spdlog::logger("OTelLogger", otel_sink));
    logger_->set_level(spdlog::level::info);
  }

  void TearDown() override {}
};

TEST_F(OpenTelemetrySinkTest, LevelToSeverity)
{
  namespace Level = spdlog::level;
  using logs_api::Severity;
  using ots = spdlog::sinks::opentelemetry_sink_st;

  ASSERT_TRUE(Severity::kFatal == ots::levelToSeverty(Level::critical));
  ASSERT_TRUE(Severity::kError == ots::levelToSeverty(Level::err));
  ASSERT_TRUE(Severity::kWarn == ots::levelToSeverty(Level::warn));
  ASSERT_TRUE(Severity::kInfo == ots::levelToSeverty(Level::info));
  ASSERT_TRUE(Severity::kDebug == ots::levelToSeverty(Level::debug));
  ASSERT_TRUE(Severity::kTrace == ots::levelToSeverty(Level::trace));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverty(Level::off));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverty(Level::n_levels));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverty(std::numeric_limits<int>::lowest()));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverty(std::numeric_limits<int>::lowest() + 1));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverty(-42));
  ASSERT_TRUE(Severity::kTrace == ots::levelToSeverty(0));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverty(42));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverty(std::numeric_limits<int>::max() - 1));
  ASSERT_TRUE(Severity::kInvalid == ots::levelToSeverty(std::numeric_limits<int>::max()));
}

TEST_F(OpenTelemetrySinkTest, Log_Success)
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
  common::AttributeValue thread_id;

  auto pre_log = std::chrono::system_clock::now();
  EXPECT_CALL(*provider_mock, GetLogger(_, _, _, _, _))
      .WillOnce(DoAll(SaveArg<0>(&logger_name), SaveArg<1>(&library_name),
                      SaveArg<2>(&library_version), Return(logger_ptr)));
  EXPECT_CALL(*logger_mock, CreateLogRecord()).WillOnce(Return(std::move(logrecord_ptr)));
  EXPECT_CALL(*logrecord_mock, SetSeverity(_)).WillOnce(SaveArg<0>(&severity));
  EXPECT_CALL(*logrecord_mock, SetBody(_)).WillOnce(SaveArg<0>(&message));
  EXPECT_CALL(*logrecord_mock, SetTimestamp(_)).WillOnce(SaveArg<0>(&timestamp));
  EXPECT_CALL(*logrecord_mock, SetAttribute(nostd::string_view("thread.id"), _))
      .WillOnce(SaveArg<1>(&thread_id));
  EXPECT_CALL(*logger_mock, EmitLogRecord(_)).Times(1);

  logger_->info("test message");
  auto post_log = std::chrono::system_clock::now();
  ASSERT_EQ(logger_name, "OTelLogger");
  ASSERT_EQ(library_name, "spdlog");
  ASSERT_EQ(library_version, spdlog::sinks::opentelemetry_sink_mt::libraryVersion());
  ASSERT_TRUE(nostd::holds_alternative<nostd::string_view>(message));
  ASSERT_EQ(nostd::get<nostd::string_view>(message), "test message");
  ASSERT_TRUE(severity == logs_api::Severity::kInfo);
  ASSERT_TRUE(timestamp.time_since_epoch() >= pre_log.time_since_epoch());
  ASSERT_TRUE(timestamp.time_since_epoch() <= post_log.time_since_epoch());
  ASSERT_TRUE(nostd::holds_alternative<uint64_t>(thread_id));
  ASSERT_GT(nostd::get<uint64_t>(thread_id), 0);
}

TEST_F(OpenTelemetrySinkTest, Log_Failure)
{
  auto provider_mock = new LoggerProviderMock();
  logs_api::Provider::SetLoggerProvider(nostd::shared_ptr<logs_api::LoggerProvider>(provider_mock));
  auto logger_mock = new LoggerMock();
  auto logger_ptr  = nostd::shared_ptr<logs_api::Logger>(logger_mock);

  nostd::string_view logger_name;
  nostd::string_view library_name;
  nostd::string_view library_version;

  EXPECT_CALL(*provider_mock, GetLogger(_, _, _, _, _))
      .WillOnce(DoAll(SaveArg<0>(&logger_name), SaveArg<1>(&library_name),
                      SaveArg<2>(&library_version), Return(logger_ptr)));
  EXPECT_CALL(*logger_mock, CreateLogRecord()).WillOnce(Return(nullptr));
  EXPECT_CALL(*logger_mock, EmitLogRecord(_)).Times(0);

  logger_->info("test message");
  ASSERT_EQ(logger_name, "OTelLogger");
  ASSERT_EQ(library_name, "spdlog");
  ASSERT_EQ(library_version, spdlog::sinks::opentelemetry_sink_mt::libraryVersion());
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

  const auto pre_log = std::chrono::system_clock::now().time_since_epoch().count();

  for (size_t index = 0; index < count; ++index)
  {
    threads.emplace_back([this, index]() { logger_->info("Test message {}", index); });
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
