// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/version.h"

#include <vector>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace geneva {
namespace metrics {

// These enums are defined in
// file: test/decoder/ifx_metrics_bin.ksy (enum metric_event_type)
enum class MetricsEventType : uint16_t {
  Uint64Metric = 50,
  DoubleScaledToLongMetric = 51,
  BatchMetric = 52,
  ExternallyAggregatedUlongMetric = 53,
  ExternallyAggregatedDoubleMetric = 54,
  DoubleMetric = 55,
  ExternallyAggregatedUlongDistributionMetric = 56,
  ExternallyAggregatedDoubleDistributionMetric = 57,
  ExternallyAggregatedDoubleScaledToLongDistributionMetric = 58,
  Undefined = 100
};

class DataTransport {
public:
  virtual bool Connect() noexcept = 0;
  virtual bool Send(MetricsEventType event_type, const char *data,
                    uint16_t length) noexcept = 0;
  virtual bool Disconnect() noexcept = 0;
  virtual ~DataTransport() = default;
};
} // namespace metrics
} // namespace geneva
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
