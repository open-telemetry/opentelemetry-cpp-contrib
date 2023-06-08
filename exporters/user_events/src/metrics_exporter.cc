// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/user_events/metrics/exporter.h"

#include "tracepoint/tracepoint.h"

#include <memory>
#include <mutex>

OPENTELEMETRY_BEGIN_NAMESPACE

namespace exporter {
namespace user_events {
namespace metrics {

// -------------------------------- Constructors --------------------------------

Exporter::Exporter() : Exporter(ExporterOptions()) {}

Exporter::Exporter(const ExporterOptions &options)
: options_(options),
    aggregation_temporality_selector_{otlp_exporter::OtlpMetricUtils::ChooseTemporalitySelector(options_.aggregation_temporality)}
{
  int err;

  err = tracepoint_open_provider(&provider_);

  err = tracepoint_connect(&otlp_metrics_, &provider_, "otlp_metrics");
  if (err) {
      OTEL_INTERNAL_LOG_ERROR("[user_events Metrics Exporter] Failed to connect to tracepoint provider");
  }
}

Exporter::~Exporter()
{
  tracepoint_close_provider(&provider_);
}

// ----------------------------- Exporter methods ------------------------------

sdk_metrics::AggregationTemporality Exporter::GetAggregationTemporality(
    sdk_metrics::InstrumentType instrument_type) const noexcept
{
  return sdk_metrics::AggregationTemporality::kDelta;
}

sdk_common::ExportResult Exporter::Export(
    const sdk_metrics::ResourceMetrics &data) noexcept
{
  auto shutdown = false;
  {
    const std::lock_guard<api_common::SpinLockMutex> locked(lock_);
    shutdown = is_shutdown_;
  }

  if (shutdown) {
    OTEL_INTERNAL_LOG_ERROR("[user_events Metrics Exporter] Exporting")

    return sdk_common::ExportResult::kFailure;
  }

  if (!TRACEPOINT_ENABLED(&otlp_metrics_))
  {
    return sdk_common::ExportResult::kSuccess;
  }

  proto::collector::metrics::v1::ExportMetricsServiceRequest request;
  otlp_exporter::OtlpMetricUtils::PopulateRequest(data, &request);

  int size = (int)request.ByteSizeLong();
  char *buffer = new char[size+sizeof(int)];
  request.SerializeToArray(buffer+sizeof(int), size);
  memcpy(buffer, &size, sizeof(int));

  struct iovec data_vecs[] = {
    {},
    { buffer, size+sizeof(int)},
  };

  int err = tracepoint_write(&otlp_metrics_, sizeof(data_vecs)/sizeof(data_vecs[0]), data_vecs);
  if (err)
  {
      OTEL_INTERNAL_LOG_ERROR("[user_events Metrics Exporter] Exporting failed with " << err);
                              
      return sdk_common::ExportResult::kFailure;
  }
  return sdk_common::ExportResult::kSuccess;
}

bool Exporter::ForceFlush(std::chrono::microseconds timeout) noexcept
{
  return true;
}

bool Exporter::Shutdown(std::chrono::microseconds time) noexcept {
  const std::lock_guard<api_common::SpinLockMutex> locked(lock_);
  if (!is_shutdown_) {
    tracepoint_close_provider(&provider_);
  }
  is_shutdown_ = true;
  return true;
}

}  // namespace metrics
}  // namespace user_events
}  // namespace exporter

OPENTELEMETRY_END_NAMESPACE