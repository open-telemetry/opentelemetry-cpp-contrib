/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <opentelemetry/instrumentation/log4cxx/appender.h>

#include <opentelemetry/logs/logger.h>
#include <opentelemetry/logs/logger_provider.h>
#include <opentelemetry/logs/provider.h>

#include <log4cxx/logger.h>

#include <gtest/gtest.h>

#include <unordered_map>

namespace common    = opentelemetry::common;
namespace nostd     = opentelemetry::nostd;
namespace logs_api  = opentelemetry::logs;
namespace trace_api = opentelemetry::trace;

TEST(OpenTelemetryAppenderTest, LevelToSeverity)
{
  using log4cxx::Level;
  using opentelemetry::logs::Severity;
  using ota = log4cxx::OpenTelemetryAppender;

  ASSERT_TRUE(Severity::kFatal == ota::levelToSeverty(Level::FATAL_INT));
  ASSERT_TRUE(Severity::kError == ota::levelToSeverty(Level::ERROR_INT));
  ASSERT_TRUE(Severity::kWarn == ota::levelToSeverty(Level::WARN_INT));
  ASSERT_TRUE(Severity::kInfo == ota::levelToSeverty(Level::INFO_INT));
  ASSERT_TRUE(Severity::kDebug == ota::levelToSeverty(Level::DEBUG_INT));
  ASSERT_TRUE(Severity::kTrace == ota::levelToSeverty(Level::TRACE_INT));
  ASSERT_TRUE(Severity::kTrace == ota::levelToSeverty(Level::ALL_INT));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverty(Level::OFF_INT));
  ASSERT_TRUE(Severity::kTrace == ota::levelToSeverty(std::numeric_limits<int>::lowest()));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverty(std::numeric_limits<int>::lowest() + 1));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverty(-42));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverty(0));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverty(42));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverty(std::numeric_limits<int>::max() - 1));
  ASSERT_TRUE(Severity::kInvalid == ota::levelToSeverty(std::numeric_limits<int>::max()));
}

struct TestLogRecord final : public logs_api::LogRecord
{
  void SetTimestamp(common::SystemTimestamp timestamp) noexcept override { timestamp_ = timestamp; }

  void SetObservedTimestamp(common::SystemTimestamp /* timestamp */) noexcept override {}

  void SetSeverity(logs_api::Severity severity) noexcept override { severity_ = severity; }

  void SetBody(const common::AttributeValue &message) noexcept override
  {
    const auto body = nostd::get_if<nostd::string_view>(&message);
    if (body)
    {
      body_ = body->data();
    }
  }

  void SetAttribute(nostd::string_view key, const common::AttributeValue &value) noexcept override
  {
    const auto entry = nostd::get_if<nostd::string_view>(&value);
    if (entry)
    {
      attributes_.emplace(key.data(), entry->data());
    }
  }

  void SetEventId(int64_t /* id */, nostd::string_view /* name */) noexcept override {}

  virtual void SetTraceId(const trace_api::TraceId & /* trace_id */) noexcept override {}

  virtual void SetSpanId(const trace_api::SpanId & /* span_id */) noexcept override {}

  virtual void SetTraceFlags(const trace_api::TraceFlags & /* trace_flags */) noexcept override {}

  common::SystemTimestamp timestamp_;
  logs_api::Severity severity_;
  std::string body_;
  std::unordered_map<std::string, std::string> attributes_;
};

struct TestLogger : public logs_api::Logger
{
  TestLogger(bool create_log_record) : create_log_record_(create_log_record) {}

  const nostd::string_view GetName() noexcept override { return "TestLogger"; }

  virtual nostd::unique_ptr<logs_api::LogRecord> CreateLogRecord() noexcept override
  {
    return create_log_record_ ? nostd::unique_ptr<logs_api::LogRecord>(new TestLogRecord())
                              : nullptr;
  }

  using Logger::EmitLogRecord;

  void EmitLogRecord(nostd::unique_ptr<logs_api::LogRecord> &&log_record) noexcept override
  {
    log_record_ = dynamic_cast<TestLogRecord *>(log_record.release());
  }

  bool create_log_record_    = false;
  TestLogRecord *log_record_ = nullptr;
};

struct TestLoggerProvider final : public logs_api::LoggerProvider
{
  TestLoggerProvider(bool create_log_record) : logger_(new TestLogger(create_log_record)) {}

  nostd::shared_ptr<logs_api::Logger> GetLogger(
      nostd::string_view logger_name,
      nostd::string_view library_name,
      nostd::string_view library_version,
      nostd::string_view /* schema_url */,
      const common::KeyValueIterable & /* attributes */) override
  {
    logger_name_     = logger_name.data();
    library_name_    = library_name.data();
    library_version_ = library_version.data();
    return nostd::shared_ptr<logs_api::Logger>(logger_);
  }

  TestLogger *logger_ = nullptr;
  std::string logger_name_;
  std::string library_name_;
  std::string library_version_;
};

TEST(OpenTelemetryAppenderTest, Append_Success)
{
  auto provider = new TestLoggerProvider(true);
  logs_api::Provider::SetLoggerProvider(nostd::shared_ptr<logs_api::LoggerProvider>(provider));
  log4cxx::spi::LocationInfo location;
  auto pre_append = std::chrono::system_clock::now();
  auto event      = std::make_shared<log4cxx::spi::LoggingEvent>(
      "test_logger", log4cxx::Level::getDebug(), "test message", location);
  auto post_append = std::chrono::system_clock::now();
  log4cxx::helpers::Pool pool;
  log4cxx::OpenTelemetryAppender ota;
  ota.append(event, pool);

  ASSERT_TRUE(provider != nullptr);
  ASSERT_EQ(provider->logger_name_, "test_logger");
  ASSERT_EQ(provider->library_name_, "log4cxx");
  ASSERT_EQ(provider->library_version_, log4cxx::OpenTelemetryAppender::libraryVersion());
  ASSERT_TRUE(provider->logger_ != nullptr);
  ASSERT_TRUE(provider->logger_->log_record_ != nullptr);

  auto log_record = provider->logger_->log_record_;
  ASSERT_EQ(log_record->body_, "test message");
  ASSERT_TRUE(log_record->severity_ == logs_api::Severity::kDebug);
  ASSERT_TRUE(log_record->timestamp_.time_since_epoch() >= pre_append.time_since_epoch());
  ASSERT_TRUE(log_record->timestamp_.time_since_epoch() <= post_append.time_since_epoch());
  ASSERT_FALSE(log_record->attributes_["thread.name"].empty());
}

TEST(OpenTelemetryAppenderTest, Append_Failure)
{
  auto provider = new TestLoggerProvider(false);
  logs_api::Provider::SetLoggerProvider(nostd::shared_ptr<logs_api::LoggerProvider>(provider));
  log4cxx::spi::LocationInfo location;
  auto event = std::make_shared<log4cxx::spi::LoggingEvent>(
      "test_logger", log4cxx::Level::getDebug(), "test message", location);
  log4cxx::helpers::Pool pool;
  log4cxx::OpenTelemetryAppender ota;
  ota.append(event, pool);

  ASSERT_TRUE(provider != nullptr);
  ASSERT_EQ(provider->logger_name_, "test_logger");
  ASSERT_EQ(provider->library_name_, "log4cxx");
  ASSERT_EQ(provider->library_version_, log4cxx::OpenTelemetryAppender::libraryVersion());
  ASSERT_TRUE(provider->logger_ != nullptr);
  ASSERT_FALSE(provider->logger_->log_record_ != nullptr);
}

TEST(OpenTelemetryAppenderTest, Configure_Logger)
{
  auto provider = new TestLoggerProvider(true);
  logs_api::Provider::SetLoggerProvider(nostd::shared_ptr<logs_api::LoggerProvider>(provider));

  auto root_logger = log4cxx::Logger::getRootLogger();
  LOG4CXX_INFO(root_logger, "This message will be ignored");
  ASSERT_TRUE(provider != nullptr);
  ASSERT_TRUE(provider->logger_ != nullptr);
  ASSERT_FALSE(provider->logger_->log_record_ != nullptr);

  auto otel_logger = log4cxx::Logger::getLogger("OTelLogger");
  otel_logger->addAppender(std::make_shared<log4cxx::OpenTelemetryAppender>());
  LOG4CXX_DEBUG(otel_logger, "This message will be processed");
  ASSERT_EQ(provider->logger_name_, "OTelLogger");
  ASSERT_EQ(provider->library_name_, "log4cxx");
  ASSERT_EQ(provider->library_version_, log4cxx::OpenTelemetryAppender::libraryVersion());
  ASSERT_TRUE(provider->logger_ != nullptr);
  ASSERT_TRUE(provider->logger_->log_record_ != nullptr);

  auto log_record = provider->logger_->log_record_;
  ASSERT_EQ(log_record->body_, "This message will be processed");
  ASSERT_TRUE(log_record->severity_ == logs_api::Severity::kDebug);
}
