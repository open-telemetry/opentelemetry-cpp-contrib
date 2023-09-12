// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

#include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace user_events
{
namespace metrics
{

namespace sdk_metrics = opentelemetry::sdk::metrics;

struct ExporterOptions
{
  opentelemetry::exporter::otlp::PreferredAggregationTemporality aggregation_temporality =
      opentelemetry::exporter::otlp::PreferredAggregationTemporality::kCumulative;
};

}  // namespace metrics
}  // namespace user_events
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
