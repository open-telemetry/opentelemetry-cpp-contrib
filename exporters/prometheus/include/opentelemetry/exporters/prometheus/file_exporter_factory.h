// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

#include "opentelemetry/exporters/prometheus/file_exporter_options.h"
#include "opentelemetry/sdk/metrics/push_metric_exporter.h"
#include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace metrics
{

/**
 * Factory class for PrometheusFileExporter.
 */
class PrometheusFileExporterFactory
{
public:
  /**
   * Create a PrometheusFileExporter using the given options.
   */
  OPENTELEMETRY_CONTRIB_PROMETHEUS_FILE_API static std::unique_ptr<
      opentelemetry::sdk::metrics::PushMetricExporter>
  Create(const PrometheusFileExporterOptions &options);
};

}  // namespace metrics
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
