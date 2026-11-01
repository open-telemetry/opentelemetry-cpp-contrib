// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/prometheus/file_exporter_factory.h"

#include <memory>

#include "opentelemetry/exporters/prometheus/file_exporter.h"
#include "opentelemetry/exporters/prometheus/file_exporter_options.h"
#include "opentelemetry/sdk/metrics/push_metric_exporter.h"
#include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace metrics
{

OPENTELEMETRY_CONTRIB_PROMETHEUS_FILE_API
    std::unique_ptr<opentelemetry::sdk::metrics::PushMetricExporter>
    PrometheusFileExporterFactory::Create(const PrometheusFileExporterOptions &options)
{
  return std::unique_ptr<opentelemetry::sdk::metrics::PushMetricExporter>(
      new PrometheusFileExporter(options));
}

}  // namespace metrics
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
