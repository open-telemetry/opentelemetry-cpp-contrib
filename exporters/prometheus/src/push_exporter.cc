// Copyright 2022, OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#  include "opentelemetry/exporters/prometheus/push_exporter.h"

OPENTELEMETRY_BEGIN_NAMESPACE

namespace exporter {
namespace metrics {
/**
 * Constructor - binds an exposer and collector to the exporter
 * @param address: an address for an exposer that exposes
 *  an HTTP endpoint for the exporter to connect to
 */
PrometheusPushExporter::PrometheusPushExporter(const PrometheusPushExporterOptions &options)
    : options_(options), is_shutdown_(false) {
  gateway_ = std::unique_ptr<::prometheus::Gateway>(new ::prometheus::Gateway{
      options_.host, options_.port, options_.jobname, options_.labels, options_.username, options_.password});
  collector_ = std::shared_ptr<::opentelemetry::exporter::metrics::PrometheusCollector>(
      new ::opentelemetry::exporter::metrics::PrometheusCollector);

  gateway_->RegisterCollectable(collector_);
}

/**
 * PrometheusPushExporter constructor with no parameters
 * Used for testing only
 */
PrometheusPushExporter::PrometheusPushExporter() : is_shutdown_(false) {
  collector_ = std::unique_ptr<::opentelemetry::exporter::metrics::PrometheusCollector>(
      new ::opentelemetry::exporter::metrics::PrometheusCollector(3));
}

::opentelemetry::sdk::metrics::AggregationTemporality PrometheusPushExporter::GetAggregationTemporality(
    ::opentelemetry::sdk::metrics::InstrumentType instrument_type) const noexcept {
  if (options_.aggregation_temporality == ::opentelemetry::sdk::metrics::AggregationTemporality::kCumulative) {
    return options_.aggregation_temporality;
  }

  switch (instrument_type) {
    case ::opentelemetry::sdk::metrics::InstrumentType::kCounter:
    case ::opentelemetry::sdk::metrics::InstrumentType::kObservableCounter:
    case ::opentelemetry::sdk::metrics::InstrumentType::kHistogram:
    case ::opentelemetry::sdk::metrics::InstrumentType::kObservableGauge:
      return ::opentelemetry::sdk::metrics::AggregationTemporality::kDelta;
    case ::opentelemetry::sdk::metrics::InstrumentType::kUpDownCounter:
    case ::opentelemetry::sdk::metrics::InstrumentType::kObservableUpDownCounter:
      return ::opentelemetry::sdk::metrics::AggregationTemporality::kCumulative;
  }
  return ::opentelemetry::sdk::metrics::AggregationTemporality::kUnspecified;
}

/**
 * Exports a batch of Metric Records.
 * @param records: a collection of records to export
 * @return: returns a ReturnCode detailing a success, or type of failure
 */
::opentelemetry::sdk::common::ExportResult PrometheusPushExporter::Export(
    const ::opentelemetry::sdk::metrics::ResourceMetrics &data) noexcept {
  if (is_shutdown_) {
    return ::opentelemetry::sdk::common::ExportResult::kFailure;
  } else if (collector_->GetCollection().size() + data.scope_metric_data_.size() >
             static_cast<size_t>(collector_->GetMaxCollectionSize())) {
    return ::opentelemetry::sdk::common::ExportResult::kFailureFull;
  } else if (data.scope_metric_data_.empty()) {
    return ::opentelemetry::sdk::common::ExportResult::kFailureInvalidArgument;
  } else {
    collector_->AddMetricData(data);
    if (gateway_) {
      int http_code = gateway_->Push();
      if (http_code >= 200 && http_code < 300) {
        return ::opentelemetry::sdk::common::ExportResult::kSuccess;
      }
      return ::opentelemetry::sdk::common::ExportResult::kFailure;
    }
    return ::opentelemetry::sdk::common::ExportResult::kSuccess;
  }
}

bool PrometheusPushExporter::ForceFlush(std::chrono::microseconds timeout) noexcept { return true; }

/**
 * Shuts down the exporter and does cleanup.
 * Since Prometheus is a pull based interface,
 * we cannot serve data remaining in the intermediate
 * collection to to client an HTTP request being sent,
 * so we flush the data.
 */
bool PrometheusPushExporter::Shutdown(std::chrono::microseconds timeout) noexcept {
  is_shutdown_ = true;

  collector_->GetCollection().clear();

  return true;
}

/**
 * @return: returns a shared_ptr to
 * the PrometheusCollector instance
 */
std::shared_ptr<::opentelemetry::exporter::metrics::PrometheusCollector> &PrometheusPushExporter::GetCollector() {
  return collector_;
}

/**
 * @return: Gets the shutdown status of the exporter
 */
bool PrometheusPushExporter::IsShutdown() const { return is_shutdown_; }

}  // namespace metrics
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
