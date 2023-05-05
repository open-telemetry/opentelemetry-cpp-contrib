// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/common/spin_lock_mutex.h"
#include "opentelemetry/exporters/geneva/metrics/connection_string_parser.h"
#include "opentelemetry/exporters/geneva/metrics/data_transport.h"
#include "opentelemetry/exporters/geneva/metrics/exporter_options.h"
#include "opentelemetry/sdk/metrics/push_metric_exporter.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace geneva {
namespace metrics {

constexpr size_t kBufferSize = 65360; // the maximum ETW payload (inclusive)
constexpr size_t kMaxDimensionNameSize = 256;
constexpr size_t kMaxDimensionValueSize = 1024;
constexpr size_t kBinaryHeaderSize = 4; // event_id (2) + body_length (2)
constexpr size_t kMetricPayloadSize =
    24; // count_dimension (2)  + reserverd_word (2) + reserverd_dword(4) +
        // timestamp_utc (8) + metric_data (8)
constexpr size_t kExternalPayloadSize =
    40; // count_dimension (2) + reserverd_word (2) + count (4) + timestamp_utc
        // (8) + metric_data_sum (8) + metric_data_min(8) + metric_data_max(8)

// time conversion constants
constexpr uint32_t kWindowsTicksPerSecond =
    10000000; // windows ticks are in 100 ns
constexpr uint64_t kSecondsToUnixTime =
    11644473600L; // number of seconds between windows epoch start
                  // 1601-01-01T00:00:00Z and UNIX/Linux epoch
                  // (1970-01-01T00:00:00Z)

const std::string kAttributeNamespaceKey = "_microsoft_metrics_namespace";
const std::string kAttributeAccountKey = "_microsoft_metrics_account";

using ValueType = nostd::variant<int64_t, double>;

/**
 * The Geneva metrics exporter exports metrics data to Geneva
 */
class Exporter final : public opentelemetry::sdk::metrics::PushMetricExporter {
public:
  Exporter(const ExporterOptions &options);

  opentelemetry::sdk::common::ExportResult
  Export(const opentelemetry::sdk::metrics::ResourceMetrics &data) noexcept
      override;

  sdk::metrics::AggregationTemporality GetAggregationTemporality(
      sdk::metrics::InstrumentType instrument_type) const noexcept override;

  bool ForceFlush(std::chrono::microseconds timeout =
                      (std::chrono::microseconds::max)()) noexcept override;

  bool Shutdown(std::chrono::microseconds timeout =
                    (std::chrono::microseconds::max)()) noexcept override;

private:
  const ExporterOptions options_;
  ConnectionStringParser connection_string_parser_;
  const sdk::metrics::AggregationTemporalitySelector
      aggregation_temporality_selector_;
  bool is_shutdown_ = false;
  mutable opentelemetry::common::SpinLockMutex lock_;
  std::unique_ptr<DataTransport> data_transport_;

  // metrics storage
  char buffer_[kBufferSize];

  size_t SerializeNonHistogramMetrics(sdk::metrics::AggregationType,
                                      MetricsEventType,
                                      const sdk::metrics::ValueType &,
                                      common::SystemTimestamp,
                                      const std::string &,
                                      const sdk::metrics::PointAttributes &);
  size_t SerializeHistogramMetrics(
      sdk::metrics::AggregationType, MetricsEventType, uint64_t,
      const sdk::metrics::ValueType &, const sdk::metrics::ValueType &,
      const sdk::metrics::ValueType &, const std::vector<double> &boundaries,
      const std::vector<uint64_t> &counts, common::SystemTimestamp,
      const std::string &, const sdk::metrics::PointAttributes &);
};

template <class T>
static void SerializeInt(char *buffer, size_t &index, T value) {
  *(reinterpret_cast<T *>(buffer + index)) = value;
  index += sizeof(T);
}

static void SerializeString(char *buffer, size_t &index,
                            const std::string &str) {
  auto size = str.size();
  SerializeInt<uint16_t>(buffer, index, static_cast<uint16_t>(size));
  if (size > 0) {
    memcpy(buffer + index, str.c_str(), size);
  }
  index += size;
}

static std::string AttributeValueToString(
    const opentelemetry::sdk::common::OwnedAttributeValue &value) {
  std::string result;
  if (nostd::holds_alternative<bool>(value)) {
    result = nostd::get<bool>(value) ? "true" : "false";
  } else if (nostd::holds_alternative<int>(value)) {
    result = std::to_string(nostd::get<int>(value));
  } else if (nostd::holds_alternative<int64_t>(value)) {
    result = std::to_string(nostd::get<int64_t>(value));
  } else if (nostd::holds_alternative<unsigned int>(value)) {
    result = std::to_string(nostd::get<unsigned int>(value));
  } else if (nostd::holds_alternative<uint64_t>(value)) {
    result = std::to_string(nostd::get<uint64_t>(value));
  } else if (nostd::holds_alternative<double>(value)) {
    result = std::to_string(nostd::get<double>(value));
  } else if (nostd::holds_alternative<std::string>(value)) {
    result = nostd::get<std::string>(value);
  } else {
    LOG_WARN("[Geneva Metrics Exporter] AttributeValueToString - "
             " Nested attributes not supported - ignored");
  }
  return result;
}

static uint64_t UnixTimeToWindowsTicks(uint64_t unix_epoch_secs) {
  uint64_t secs_since_windows_epoch = unix_epoch_secs + kSecondsToUnixTime;
  return (secs_since_windows_epoch * (uint64_t)kWindowsTicksPerSecond);
}

} // namespace metrics
} // namespace geneva
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE