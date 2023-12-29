/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <opentelemetry/instrumentation/log4cxx/appender.h>

#include <opentelemetry/logs/provider.h>
#include <opentelemetry/trace/semantic_conventions.h>

namespace log4cxx
{

void OpenTelemetryAppender::append(const spi::LoggingEventPtr &event, helpers::Pool &)
{
  using namespace opentelemetry::trace::SemanticConventions;
  static constexpr auto kLibraryName = "log4cxx";

  auto provider   = opentelemetry::logs::Provider::GetLoggerProvider();
  auto logger     = provider->GetLogger(event->getLoggerName(), kLibraryName, libraryVersion());
  auto log_record = logger->CreateLogRecord();

  if (log_record)
  {
    log_record->SetSeverity(levelToSeverity(event->getLevel()->toInt()));
    log_record->SetBody(event->getMessage());
    log_record->SetTimestamp(event->getChronoTimeStamp());
    log_record->SetAttribute(kThreadName, event->getThreadName());
    logger->EmitLogRecord(std::move(log_record));
  }
}

}  // namespace log4cxx
