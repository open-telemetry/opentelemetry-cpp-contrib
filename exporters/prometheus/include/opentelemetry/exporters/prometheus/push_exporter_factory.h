// Copyright 2023, OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

#include "opentelemetry/exporters/prometheus/push_exporter_options.h"
#include "opentelemetry/sdk/metrics/push_metric_exporter.h"
#include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace metrics
{

/**
 * Factory class for PrometheusExporter.
 */
class PrometheusPushExporterFactory
{
public:
  /**
   * Create a PrometheusExporter using the given options.
   */
  OPENTELEMETRY_CONTRIB_PROMETHEUS_PUSH_API static std::unique_ptr<
      opentelemetry::sdk::metrics::PushMetricExporter>
  Create(const PrometheusPushExporterOptions &options);
};

}  // namespace metrics
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
