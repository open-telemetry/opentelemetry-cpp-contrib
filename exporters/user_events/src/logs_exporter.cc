// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_LOGS_PREVIEW

#  include "opentelemetry/exporters/user_events/logs/exporter.h"
#  include "opentelemetry/sdk_config.h"

namespace nostd     = opentelemetry::nostd;
namespace sdklogs   = opentelemetry::sdk::logs;
namespace sdkcommon = opentelemetry::sdk::common;

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace user_events
{
namespace logs
{

/*********************** Constructor ***********************/

Exporter::Exporter(const ExporterOptions &options) noexcept : options_(options)
{
}

/*********************** Exporter methods ***********************/

std::unique_ptr<sdk_logs::Recordable> Exporter::MakeRecordable() noexcept
{
  // return std::unique_ptr<Recordable>(new Recordable());
  return nullptr;
}

sdk::common::ExportResult Exporter::Export(
    const nostd::span<std::unique_ptr<sdklogs::Recordable>> &records) noexcept
{
  if (isShutdown())
  {
    OTEL_INTERNAL_LOG_ERROR("[user_events Log Exporter] Exporting "
                            << records.size() << " log(s) failed, exporter is shutdown");
    return sdk::common::ExportResult::kFailure;
  }

  ehd::EventBuilder eb;
  int err;

  for (auto &record : records)
  {
    eb.Reset("opentelemetry-logs", 0);
    // TODO: set Id and Version to something meaningful
    eb.IdVersion(1, 2);
    // eb.AddString<char>("str", "Body", record->GetBody());

    err = eb.Write(*event_set_);

    if (err != 0)
    {
      OTEL_INTERNAL_LOG_ERROR("[user_events Log Exporter] Exporting failed, error code: " << err);
      return sdk::common::ExportResult::kFailure;
    }
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
}  // namespace user_events
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE

#endif