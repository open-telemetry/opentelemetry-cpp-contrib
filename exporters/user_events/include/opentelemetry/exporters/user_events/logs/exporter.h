// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef ENABLE_LOGS_PREVIEW

#  include "exporter_options.h"
#  include "opentelemetry/common/spin_lock_mutex.h"
#  include "opentelemetry/sdk/logs/exporter.h"

#  include <eventheader/EventHeaderDynamic.h>
#  include <array>
#  include <mutex>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace user_events
{
namespace logs
{

namespace sdk_logs = opentelemetry::sdk::logs;

/**
 * The user_events logs exporter exports logs data to tracepoint via user_events.
 */
class Exporter final : public opentelemetry::sdk::logs::LogRecordExporter
{
public:
  explicit Exporter(const ExporterOptions &options) noexcept;

  /**
   * Exports a span of logs sent from the processor.
   */

  std::unique_ptr<sdk_logs::Recordable> MakeRecordable() noexcept override;

  opentelemetry::sdk::common::ExportResult Export(
      const opentelemetry::nostd::span<std::unique_ptr<sdk_logs::Recordable>> &records) noexcept
      override;

  bool Shutdown(
      std::chrono::microseconds timeout = (std::chrono::microseconds::max)()) noexcept override;

  bool isShutdown() const noexcept;

private:
  const ExporterOptions options_;
  bool is_shutdown_ = false;
  mutable opentelemetry::common::SpinLockMutex lock_;

  ehd::Provider provider_{"opentelemetry_logs"};
  std::array<std::shared_ptr<const ehd::EventSet>, 6> event_set_levels_ = {
      provider_.RegisterSet(static_cast<event_level>(6), 1),
      provider_.RegisterSet(event_level_verbose, 1),
      provider_.RegisterSet(event_level_information, 1),
      provider_.RegisterSet(event_level_warning, 1),
      provider_.RegisterSet(event_level_error, 1),
      provider_.RegisterSet(event_level_critical_error, 1)};

};  // class Exporter

}  // namespace logs
}  // namespace user_events
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE

#endif