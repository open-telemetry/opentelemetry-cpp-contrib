// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_LOGS_PREVIEW

#  include "opentelemetry/exporters/userevents/logs/exporter.h"
#  include "opentelemetry/sdk_config.h"

namespace nostd     = opentelemetry::nostd;
namespace sdklogs   = opentelemetry::sdk::logs;
namespace sdkcommon = opentelemetry::sdk::common;

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace logs
{

/*********************** Constructor ***********************/

Exporter::Exporter(const ExporterOptions &options) noexcept : options_(options)
{
  // TODO register TraceLoggingProvider
}

/*********************** Exporter methods ***********************/
sdk::common::ExportResult Exporter::Export(
    const nostd::span<std::unique_ptr<sdklogs::Recordable>> &records) noexcept
{
  if (isShutdown())
  {
    OTEL_INTERNAL_LOG_ERROR("[UserEvents Log Exporter] Exporting "
                            << records.size() << " log(s) failed, exporter is shutdown");
    return sdk::common::ExportResult::kFailure;
  }

  for (auto &record : records)
  {
    // TODO: serialize and write record to user_events
    TraceLoggingWrite(
      g_hProvider,
      "opentelemetry-logs",
      TraceLoggingString(record->GetBody(), "Body"));
  }

  return sdk::common::ExportResult::kSuccess;
}

bool Exporter::Shutdown(std::chrono::microseconds) noexcept
{
  const std::lock_guard<opentelemetry::common::SpinLockMutex> locked(lock_);
  is_shutdown_ = true;
  return true;
}

bool Exporter::isShutdown() const noexcept
{
  const std::lock_guard<opentelemetry::common::SpinLockMutex> locked(lock_);
  return is_shutdown_;
}


}  // namespace logs
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE

#endif