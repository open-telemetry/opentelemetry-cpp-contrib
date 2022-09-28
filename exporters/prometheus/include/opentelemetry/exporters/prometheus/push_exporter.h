// Copyright 2022, OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#  ifdef _WIN32
#    include <io.h>        // NOLINT
#    include <winsock2.h>  // NOLINT
#  else
#    include <unistd.h>  // NOLINT
#  endif

#  include <chrono>
#  include <memory>
#  include <string>
#  include <vector>

#  include "prometheus/gateway.h"

#  include "opentelemetry/exporters/prometheus/collector.h"
#  include "opentelemetry/nostd/span.h"
#  include "opentelemetry/sdk/common/env_variables.h"
#  include "opentelemetry/sdk/metrics/metric_exporter.h"
#  include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace metrics {
/**
 * Struct to hold Prometheus exporter options.
 */
struct PrometheusPushExporterOptions {
  std::string host;
  std::string port;
  std::string jobname;
  ::prometheus::Labels labels;
  std::string username;
  std::string password;

  ::opentelemetry::sdk::metrics::AggregationTemporality aggregation_temporality =
      ::opentelemetry::sdk::metrics::AggregationTemporality::kDelta;
};

class PrometheusPushExporter : public ::opentelemetry::sdk::metrics::MetricExporter {
 public:
  /**
   * Constructor - binds an exposer and collector to the exporter
   * @param options: options for an exposer that exposes
   *  an HTTP endpoint for the exporter to connect to
   */
  explicit PrometheusPushExporter(const PrometheusPushExporterOptions &options);

  /**
   * Get the AggregationTemporality for Prometheus exporter
   *
   * @return AggregationTemporality
   */
  ::opentelemetry::sdk::metrics::AggregationTemporality GetAggregationTemporality(
      ::opentelemetry::sdk::metrics::InstrumentType instrument_type) const noexcept override;

  /**
   * Exports a batch of Metric Records.
   * @param records: a collection of records to export
   * @return: returns a ReturnCode detailing a success, or type of failure
   */
  ::opentelemetry::sdk::common::ExportResult Export(
      const ::opentelemetry::sdk::metrics::ResourceMetrics &data) noexcept override;

  /**
   * Force flush the exporter.
   */
  bool ForceFlush(std::chrono::microseconds timeout = (std::chrono::microseconds::max)()) noexcept override;

  /**
   * Shuts down the exporter and does cleanup.
   * Since Prometheus is a pull based interface,
   * we cannot serve data remaining in the intermediate
   * collection to to client an HTTP request being sent,
   * so we flush the data.
   */
  bool Shutdown(std::chrono::microseconds timeout = std::chrono::microseconds(0)) noexcept override;

  /**
   * @return: returns a shared_ptr to
   * the PrometheusCollector instance
   */
  std::shared_ptr<::opentelemetry::exporter::metrics::PrometheusCollector> &GetCollector();

  /**
   * @return: Gets the shutdown status of the exporter
   */
  bool IsShutdown() const;

 private:
  // The configuration options associated with this exporter.
  const PrometheusPushExporterOptions options_;
  /**
   * exporter shutdown status
   */
  bool is_shutdown_;

  /**
   * Pointer to a
   * PrometheusCollector instance
   */
  std::shared_ptr<::opentelemetry::exporter::metrics::PrometheusCollector> collector_;

  /**
   * Pointer to an
   * Gateway instance
   */
  std::unique_ptr<::prometheus::Gateway> gateway_;

  /**
   * friend class for testing
   */
  friend class PrometheusPushExporterTest;

  /**
   * PrometheusPushExporter constructor with no parameters
   * Used for testing only
   */
  PrometheusPushExporter();
};

}  // namespace metrics
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
