// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef ENABLE_LOGS_PREVIEW

#  include "opentelemetry/common/spin_lock_mutex.h"
#  include "opentelemetry/sdk/logs/exporter.h"
#  include "opentelemetry/exporters/userevents/logs/exporter_options.h"

#  include <TraceLoggingProvider.h>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace userevents {
namespace logs {

TRACELOGGING_DEFINE_PROVIDER(  // defines g_hProvider
    g_hProvider,               // Name of the provider variable
    "OpenTelemetry-Logs-Provider"); // Human-readable name of the provider

/**
 * The user_events logs exporter exports logs data to Geneva
 */
class Exporter final : public opentelemetry::sdk::logs::LogRecordExporter {
public:
  Exporter(const ExporterOptions &options);

  /**
   * Exports a span of logs sent from the processor.
   */
  opentelemetry::sdk::common::ExportResult Export(
      const opentelemetry::nostd::span<std::unique_ptr<sdk::logs::Recordable>> &records) noexcept
      override;

  bool ForceFlush(std::chrono::microseconds timeout =
                      (std::chrono::microseconds::max)()) noexcept override;

  bool Shutdown(std::chrono::microseconds timeout =
                    (std::chrono::microseconds::max)()) noexcept override;

private:
  const ExporterOptions options_;
  bool is_shutdown_ = false;
  mutable opentelemetry::common::SpinLockMutex lock_;
};

} // namespace logs
} // namespace userevents
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE

#endif