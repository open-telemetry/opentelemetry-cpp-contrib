/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <opentelemetry/instrumentation/glog/sink.h>

#include <opentelemetry/logs/provider.h>
#include <opentelemetry/trace/semantic_conventions.h>
#include <opentelemetry/version.h>

#include <chrono>

namespace google
{

#if defined(GLOG_VERSION_HAS_LOGMESSAGETIME)
void OpenTelemetrySink::send(google::LogSeverity severity,
                             const char *full_filename,
                             const char * /* base_filename */,
                             int line,
                             const google::LogMessageTime &logmsgtime,
                             const char *message,
                             size_t message_len)
{
#else
void OpenTelemetrySink::send(google::LogSeverity severity,
                             const char *full_filename,
                             const char * /* base_filename */,
                             int line,
                             const struct std::tm * /* time_tm */,
                             const char *message,
                             size_t message_len)
{
  // Compensate for the lack of precision in older versions
  const auto timestamp = std::chrono::system_clock::now();
#endif

  static constexpr auto kLoggerName  = "Google logger";
  static constexpr auto kLibraryName = "glog";

  auto provider   = opentelemetry::logs::Provider::GetLoggerProvider();
  auto logger     = provider->GetLogger(kLoggerName, kLibraryName);
  auto log_record = logger->CreateLogRecord();

  if (log_record)
  {
    using namespace opentelemetry::trace::SemanticConventions;
    using namespace std::chrono;

#if defined(GLOG_VERSION_HAS_LOGMESSAGETIME)
    static constexpr auto secs_to_msecs = duration_cast<microseconds>(seconds{1}).count();
    const auto timestamp = microseconds(secs_to_msecs * logmsgtime.timestamp() + logmsgtime.usec());
#endif

    log_record->SetSeverity(levelToSeverity(severity));
    log_record->SetBody(opentelemetry::nostd::string_view(message, message_len));
    log_record->SetTimestamp(system_clock::time_point(timestamp));
    log_record->SetAttribute(kCodeFilepath, full_filename);
    log_record->SetAttribute(kCodeLineno, line);
    logger->EmitLogRecord(std::move(log_record));
  }
}

}  // namespace google
