// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstddef>
#include <memory>

#include "opentelemetry/exporters/prometheus/file_exporter_options.h"
#include "opentelemetry/sdk/metrics/push_metric_exporter.h"
#include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace metrics
{

class PrometheusFileBackend;

/**
 * The Prometheus file exporter periodically serializes the collected metrics to
 * rotating files using the Prometheus text format.
 */
class PrometheusFileExporter : public ::opentelemetry::sdk::metrics::PushMetricExporter
{
public:
  /**
   * Constructor - creates the file backend for the exporter.
   * @param options: options for the file exporter
   */
  explicit PrometheusFileExporter(const PrometheusFileExporterOptions &options);

  /**
   * Get the AggregationTemporality for Prometheus exporter
   *
   * @return AggregationTemporality
   */
  ::opentelemetry::sdk::metrics::AggregationTemporality GetAggregationTemporality(
      ::opentelemetry::sdk::metrics::InstrumentType instrument_type) const noexcept override;

  /**
   * Exports a batch of Metric Records.
   * @param data: a collection of records to export
   * @return: returns a ReturnCode detailing a success, or type of failure
   */
  ::opentelemetry::sdk::common::ExportResult Export(
      const ::opentelemetry::sdk::metrics::ResourceMetrics &data) noexcept override;

  /**
   * Force flush the exporter.
   */
  bool ForceFlush(
      std::chrono::microseconds timeout = (std::chrono::microseconds::max)()) noexcept override;

  /**
   * Shuts down the exporter and does cleanup.
   */
  bool Shutdown(std::chrono::microseconds timeout = std::chrono::microseconds(0)) noexcept override;

  /**
   * @return: Gets the shutdown status of the exporter
   */
  bool IsShutdown() const;

private:
  // The configuration options associated with this exporter.
  const PrometheusFileExporterOptions options_;

  // exporter shutdown status
  bool is_shutdown_;

  std::shared_ptr<PrometheusFileBackend> backend_;
};

}  // namespace metrics
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
