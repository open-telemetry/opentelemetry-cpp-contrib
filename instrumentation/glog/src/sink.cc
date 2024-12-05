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

void OpenTelemetrySink::send(google::LogSeverity severity,
                             const char *full_filename,
                             const char * /* base_filename */,
                             int line,
                             const google::LogMessageTime &logmsgtime,
                             const char *message,
                             size_t message_len)
{

  static constexpr auto kLoggerName  = "Google logger";
  static constexpr auto kLibraryName = "glog";

  auto provider   = opentelemetry::logs::Provider::GetLoggerProvider();
  auto logger     = provider->GetLogger(kLoggerName, kLibraryName);
  auto log_record = logger->CreateLogRecord();

  if (log_record)
  {
    using namespace opentelemetry::trace::SemanticConventions;
    using namespace std::chrono;

    log_record->SetSeverity(levelToSeverity(severity));
    log_record->SetBody(opentelemetry::nostd::string_view(message, message_len));
    log_record->SetTimestamp(logmsgtime.when());
    log_record->SetAttribute(kCodeFilepath, full_filename);
    log_record->SetAttribute(kCodeLineno, line);
    logger->EmitLogRecord(std::move(log_record));
  }
}

}  // namespace google
