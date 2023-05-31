// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/version.h"
#include <string>

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
  sdk_metrics::AggregationTemporality aggregation_temporality = sdk_metrics::AggregationTemporality::kCumulative;
};

} // namespace metrics
} // namespace user_events
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE