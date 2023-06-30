// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/exporters/otlp/otlp_metric_utils.h"

#include "exporter_options.h"
#include "opentelemetry/sdk/metrics/push_metric_exporter.h"

#include "opentelemetry/common/spin_lock_mutex.h"
#include "opentelemetry/sdk/common/global_log_handler.h"

#include <tracepoint/tracepoint.h>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace user_events
{
namespace metrics
{

namespace api_common = opentelemetry::common;
namespace sdk_common = opentelemetry::sdk::common;
namespace sdk_metrics = opentelemetry::sdk::metrics;
namespace otlp_exporter = opentelemetry::exporter::otlp;

class Exporter final : public opentelemetry::sdk::metrics::PushMetricExporter
{
public:
  /**
   * Create a metrics exporter using all default options.
   */
  Exporter();

  explicit Exporter(const ExporterOptions &options);

  ~Exporter() override;

  /**
   * Get the a`AggregationTeporality for this exporter.
   * 
   * @return AggregationTemporality
   */
  sdk_metrics::AggregationTemporality GetAggregationTemporality(
    sdk_metrics::InstrumentType instrument_type) const noexcept override;

  sdk_common::ExportResult Export(
    const sdk_metrics::ResourceMetrics &data) noexcept override;

  bool ForceFlush(
    std::chrono::microseconds timeout = std::chrono::microseconds::max()) noexcept override;

  bool Shutdown(
    std::chrono::microseconds timeout = std::chrono::microseconds::max()) noexcept override;

  private:
    // The configration options associated with this exporter.
    const ExporterOptions options_;
    mutable opentelemetry::common::SpinLockMutex lock_;
    const sdk_metrics::AggregationTemporalitySelector aggregation_temporality_selector_;

    bool is_shutdown_ = false;

    // A tracepoint_provider_state represents a connection to the tracing system.
    // It is usually global so that it can be shared between exporters.
    tracepoint_provider_state provider_ = TRACEPOINT_PROVIDER_STATE_INIT;
    tracepoint_state otlp_metrics_ = TRACEPOINT_STATE_INIT;

};

}  // namespace metrics
}  // namespace user_events
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE