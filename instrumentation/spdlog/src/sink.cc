/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <opentelemetry/instrumentation/spdlog/sink.h>

#include <opentelemetry/logs/provider.h>
#include <opentelemetry/trace/semantic_conventions.h>
#include <opentelemetry/version.h>

namespace spdlog
{
namespace sinks
{

template <typename Mutex>
void OpenTelemetrySink<Mutex>::sink_it_(const spdlog::details::log_msg &msg)
{
  static constexpr auto kLibraryName = "spdlog";

  auto provider   = opentelemetry::logs::Provider::GetLoggerProvider();
  auto logger     = provider->GetLogger(msg.logger_name.data(), kLibraryName, libraryVersion());
  auto log_record = logger->CreateLogRecord();

  if (log_record)
  {
    using namespace opentelemetry::trace::SemanticConventions;

    log_record->SetSeverity(levelToSeverity(msg.level));
    log_record->SetBody(opentelemetry::nostd::string_view(msg.payload.data()));
    log_record->SetTimestamp(msg.time);
    if (!msg.source.empty())
    {
      log_record->SetAttribute(kCodeFilepath, msg.source.filename);
      log_record->SetAttribute(kCodeLineno, msg.source.line);
    }
    log_record->SetAttribute(kThreadId, msg.thread_id);
    logger->EmitLogRecord(std::move(log_record));
  }
}

// Explicit instantiation to avoid linker errors
template class OpenTelemetrySink<std::mutex>;
template class OpenTelemetrySink<spdlog::details::null_mutex>;

}  // namespace sinks
}  // namespace spdlog
