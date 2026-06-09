// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include "opentelemetry/version.h"

#include <vector>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace statsd {
namespace metrics {

// Metrics Type defined in Statsd Protocol
// Other statsd metric types, such as c ("c"ounter), t ("t"imer), m ("m"eters), h ("h"istograms), etc are not supported, typically because such functionality is implemented differently in MDM.
enum class MetricsEventType {
  // g ("g"auge) is a standard metric with a single 64-bit integer value; <value> is an integer number.
  Gauge,

  // s ("s"caled fixed-point number) is used to represent fixed-point values (such as CPU usages, load averages, fractions, etc). 
  // Precision for a fixed-point metric is determined by a precision setting in per-metric setup. <value> can be any fractional number, but note that the actual value would in Geneva Metrics be truncated to a specified precision and still stored in 64-bit container.
  ScaledFixedPointNumber,

  // f ("f"loating point number) is used to represent floating point numbers which vary a lot in its magnitude and thus warrant storage in a mantissa + expontent format (such as physical quantities, results of complex calculations, etc). <value> can be any fractional number, which would be stored as double precision IEEE 754 float.
  FloatingPointNumber,

  Unknown
};

inline std::string MetricsEventTypeToString(MetricsEventType type) {
  switch (type) {
    case MetricsEventType::Gauge:
      return "g";
    case MetricsEventType::ScaledFixedPointNumber:
      return "s";
    case MetricsEventType::FloatingPointNumber:
      return "f";
    default:
      return "";      
  }
}

class DataTransport {
public:
  virtual bool Connect() noexcept = 0;
  virtual bool Send(MetricsEventType event_type, const char *data,
                    uint16_t length) noexcept = 0;
  virtual bool Disconnect() noexcept = 0;
  virtual ~DataTransport() = default;
};
} // namespace metrics
} // namespace statsd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
