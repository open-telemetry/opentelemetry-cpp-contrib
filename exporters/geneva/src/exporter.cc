// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/geneva/metrics/exporter.h"
#include "opentelemetry/exporters/geneva/metrics/macros.h"
#include "opentelemetry/exporters/geneva/metrics/unix_domain_socket_data_transport.h"
#ifdef _WIN32
#include "opentelemetry/exporters/geneva/metrics/etw_data_transport.h"
#endif
#include "opentelemetry/metrics/meter_provider.h"
#include "opentelemetry/sdk_config.h"

#include <memory>
#include <mutex>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace geneva {
namespace metrics {
Exporter::Exporter(const ExporterOptions &options)
    : options_(options), connection_string_parser_(options_.connection_string),
      data_transport_{nullptr} {
  if (connection_string_parser_.IsValid()) {
    if (connection_string_parser_.transport_protocol_ ==
        TransportProtocol::kUNIX) {
      data_transport_ =
          std::unique_ptr<DataTransport>(new UnixDomainSocketDataTransport(
              connection_string_parser_.connection_string_));
    }
#ifdef _WIN32
    else if (connection_string_parser_.transport_protocol_ ==
             TransportProtocol::kETW) {
      data_transport_ = std::unique_ptr<DataTransport>(
          new ETWDataTransport(kBinaryHeaderSize));
    }
#endif
  }
  // Connect transport at initialization
  auto status = data_transport_->Connect();
  if (!status) {
    LOG_ERROR("[Geneva Exporter] Connect failed. No data would be sent.");
    is_shutdown_ = true;
    return;
  }
}

sdk::metrics::AggregationTemporality Exporter::GetAggregationTemporality(
    sdk::metrics::InstrumentType instrument_type) const noexcept {
  return sdk::metrics::AggregationTemporality::kDelta;
}

opentelemetry::sdk::common::ExportResult Exporter::Export(
    const opentelemetry::sdk::metrics::ResourceMetrics &data) noexcept {
  auto shutdown = false;
  {
    const std::lock_guard<opentelemetry::common::SpinLockMutex> locked(lock_);
    shutdown = is_shutdown_;
  }
  if (shutdown) {
    OTEL_INTERNAL_LOG_ERROR("[Genava Exporter] Exporting "
                            << data.scope_metric_data_.size()
                            << " metric(s) failed, exporter is shutdown");
    return sdk::common::ExportResult::kFailure;
  }

  if (data.scope_metric_data_.empty()) {
    return sdk::common::ExportResult::kSuccess;
  }

  for (auto &record : data.scope_metric_data_) {
    for (const auto &metric_data : record.metric_data_) {
      for (auto &point_data_with_attributes : metric_data.point_data_attr_) {
        size_t body_length = 0;
        if (nostd::holds_alternative<sdk::metrics::SumPointData>(
                point_data_with_attributes.point_data)) {
          auto value = nostd::get<sdk::metrics::SumPointData>(
              point_data_with_attributes.point_data);
          ValueType new_value = value.value_;

          MetricsEventType event_type;

          if (nostd::holds_alternative<double>(value.value_)) {
            event_type = MetricsEventType::DoubleMetric;
          } else {
            if (!value.is_monotonic_) {
              // NOTE - Potential for minor precision loss implicitly going from
              // int64_t to double -
              //   - A 64-bit integer can hold more significant decimal digits
              //   than a standard
              //     IEEE (64-bit) double precision floating-point
              //     representation
              new_value = static_cast<double>(nostd::get<int64_t>(new_value));
              event_type = MetricsEventType::DoubleMetric;

            } else {
              event_type = MetricsEventType::Uint64Metric;
            }
          }
          body_length = SerializeNonHistogramMetrics(
              sdk::metrics::AggregationType::kSum, event_type, new_value,
              metric_data.end_ts, metric_data.instrument_descriptor.name_,
              point_data_with_attributes.attributes);
          data_transport_->Send(event_type, buffer_,
                                body_length + kBinaryHeaderSize);

        } else if (nostd::holds_alternative<sdk::metrics::LastValuePointData>(
                       point_data_with_attributes.point_data)) {
          auto value = nostd::get<sdk::metrics::LastValuePointData>(
              point_data_with_attributes.point_data);
          ValueType new_value = value.value_;
          if (nostd::holds_alternative<int64_t>(value.value_)) {
            // NOTE - Potential for minor precision loss implicitly going from
            // int64_t to double -
            //   - A 64-bit integer can hold more significant decimal digits
            //   than a standard
            //     IEEE (64-bit) double precision floating-point representation
            new_value = static_cast<double>(nostd::get<int64_t>(new_value));
          }
          MetricsEventType event_type = MetricsEventType::DoubleMetric;
          body_length = SerializeNonHistogramMetrics(
              sdk::metrics::AggregationType::kLastValue, event_type, new_value,
              metric_data.end_ts, metric_data.instrument_descriptor.name_,
              point_data_with_attributes.attributes);
          data_transport_->Send(event_type, buffer_,
                                body_length + kBinaryHeaderSize);
        } else if (nostd::holds_alternative<sdk::metrics::HistogramPointData>(
                       point_data_with_attributes.point_data)) {
          auto value = nostd::get<sdk::metrics::HistogramPointData>(
              point_data_with_attributes.point_data);
          ValueType new_sum = value.sum_;
          ValueType new_min = value.min_;
          ValueType new_max = value.max_;

          if (nostd::holds_alternative<double>(value.sum_)) {
            // TODO: Double is not supported by Geneva, convert it to int64_t
            new_sum = static_cast<int64_t>(nostd::get<double>(new_sum));
            new_min = static_cast<int64_t>(nostd::get<double>(new_min));
            new_max = static_cast<int64_t>(nostd::get<double>(new_max));
          }
          MetricsEventType event_type =
              MetricsEventType::ExternallyAggregatedUlongDistributionMetric;
          body_length = SerializeHistogramMetrics(
              sdk::metrics::AggregationType::kHistogram, event_type,
              value.count_, new_sum, new_min, new_max,
              nostd::get<sdk::metrics::HistogramPointData>(
                  point_data_with_attributes.point_data)
                  .boundaries_,
              nostd::get<sdk::metrics::HistogramPointData>(
                  point_data_with_attributes.point_data)
                  .counts_,
              metric_data.end_ts, metric_data.instrument_descriptor.name_,
              point_data_with_attributes.attributes);
          data_transport_->Send(event_type, buffer_,
                                body_length + kBinaryHeaderSize);
        }
      }
    }
  }
  return opentelemetry::sdk::common::ExportResult::kSuccess;
}

bool Exporter::ForceFlush(std::chrono::microseconds timeout) noexcept {
  return true;
}

bool Exporter::Shutdown(std::chrono::microseconds timeout) noexcept {
  const std::lock_guard<opentelemetry::common::SpinLockMutex> locked(lock_);
  is_shutdown_ = true;
  return true;
}

size_t Exporter::SerializeNonHistogramMetrics(
    sdk::metrics::AggregationType agg_type, MetricsEventType event_type,
    const sdk::metrics::ValueType &value, common::SystemTimestamp ts,
    const std::string &metric_name,
    const sdk::metrics::PointAttributes &attributes) {

  // The buffer format is as follows:
  // -- BinaryHeader
  // -- MetricPayload
  // -- Variable length content

  // Leave enough space for the header and fixed payload
  auto bufferIndex = kBinaryHeaderSize + kMetricPayloadSize;

  auto account_name = connection_string_parser_.account_;
  auto account_namespace = connection_string_parser_.namespace_;

  // try reading namespace and/or account from attributes
  // TBD = This can be avoided by migrating to  the 
  // TLV binary format
  for (const auto &kv : attributes) {
    if (kv.first == kAttributeAccountKey){
      account_name = AttributeValueToString(kv.second); 
    }
    else if (kv.first == kAttributeNamespaceKey) {
      account_namespace = AttributeValueToString(kv.second); 
    }
  }

  // account name
  SerializeString(buffer_, bufferIndex, account_name);
  // namespace
  SerializeString(buffer_, bufferIndex, account_namespace);
  // metric name
  SerializeString(buffer_, bufferIndex, metric_name);

  uint16_t attributes_size = 0;
  for (const auto &kv : attributes) {
    if (kv.first.size() > kMaxDimensionNameSize) {
      LOG_WARN("Dimension name limit overflow: %s Limit: %d", kv.first.c_str(),
               kMaxDimensionNameSize);
      continue;
    }
    if (kv.first == kAttributeAccountKey ||
        kv.first == kAttributeNamespaceKey)
    {
      // custom namespace and account name should't be exported
      continue;
    }
    attributes_size++;
    SerializeString(buffer_, bufferIndex, kv.first);
  }
  for (const auto &kv : attributes) {
    if (kv.first.size() > kMaxDimensionNameSize) {
      LOG_WARN("Dimension name limit overflow: %s Limit: %d", kv.first.c_str(),
               kMaxDimensionNameSize);
      continue;
    }
    if (kv.first == kAttributeAccountKey ||
        kv.first == kAttributeNamespaceKey)
    {
      // custom namespace and account name should't be exported
      continue;
    }
    auto attr_value = AttributeValueToString(kv.second);
    SerializeString(buffer_, bufferIndex, attr_value);
  }
  // length zero for auto-pilot
  SerializeInt<uint16_t>(buffer_, bufferIndex, 0);

  // get final size of payload to be added in front of buffer
  uint16_t body_length = bufferIndex - kBinaryHeaderSize;

  // Add rest of the fields in front of buffer
  bufferIndex = 0;

  // event_type
  SerializeInt<uint16_t>(buffer_, bufferIndex,
                         static_cast<uint16_t>(event_type));

  // body length
  SerializeInt<uint16_t>(buffer_, bufferIndex,
                         static_cast<uint16_t>(body_length));

  // count of dimensions.
  SerializeInt<uint16_t>(buffer_, bufferIndex,
                         static_cast<uint16_t>(attributes_size));

  // reserverd word (2 bytes)
  SerializeInt<uint16_t>(buffer_, bufferIndex, 0);

  // reserved word (4 bytes)
  SerializeInt<uint32_t>(buffer_, bufferIndex, 0);

  // timestamp utc (8 bytes)
  auto windows_ticks = UnixTimeToWindowsTicks(
      std::chrono::duration_cast<std::chrono::duration<std::uint64_t>>(
          ts.time_since_epoch())
          .count());

  SerializeInt<uint64_t>(buffer_, bufferIndex, windows_ticks);
  if (event_type == MetricsEventType::Uint64Metric) {
    SerializeInt<uint64_t>(buffer_, bufferIndex,
                           static_cast<uint64_t>(nostd::get<int64_t>(value)));
  } else if (event_type == MetricsEventType::DoubleMetric) {
    SerializeInt<uint64_t>(
        buffer_, bufferIndex,
        *(reinterpret_cast<const uint64_t *>(&(nostd::get<double>(value)))));
  } else {
    // Won't reach here.
  }
  return body_length;
}

size_t Exporter::SerializeHistogramMetrics(
    sdk::metrics::AggregationType agg_type, MetricsEventType event_type,
    uint64_t count, const sdk::metrics::ValueType &sum,
    const sdk::metrics::ValueType &min, const sdk::metrics::ValueType &max,
    const std::vector<double> &boundaries, const std::vector<uint64_t> &counts,
    common::SystemTimestamp ts, const std::string &metric_name,
    const sdk::metrics::PointAttributes &attributes) {

  // The buffer format is as follows:
  // -- BinaryHeader
  // -- ExternalPayload
  // -- Variable length content

  // Leave enough space for the header and fixed payload
  auto bufferIndex = kBinaryHeaderSize + kExternalPayloadSize;

  auto account_name = connection_string_parser_.account_;
  auto account_namespace = connection_string_parser_.namespace_;

  // try reading namespace and/or account from attributes
  // TODO: This can be avoided by migrating to the 
  // TLV binary format
  for (const auto &kv : attributes) {
    if (kv.first  == kAttributeAccountKey){
      account_name = AttributeValueToString(kv.second); 
    }
    else if (kv.first ==  kAttributeNamespaceKey) {
      account_namespace = AttributeValueToString(kv.second); 
    }
  }

  // account name
  SerializeString(buffer_, bufferIndex, account_name);
  // namespace
  SerializeString(buffer_, bufferIndex, account_namespace);
  // metric name
  SerializeString(buffer_, bufferIndex, metric_name);

  uint16_t attributes_size = 0;
  // dimentions - name
  for (const auto &kv : attributes) {
    if (kv.first.size() > kMaxDimensionNameSize) {
      LOG_WARN("Dimension name limit overflow: %s Limit: %d", kv.first.c_str(),
               kMaxDimensionNameSize);
      continue;
    }
    if (kv.first == kAttributeAccountKey ||
        kv.first == kAttributeNamespaceKey)
    {
      // custom namespace and account name should't be exported
      continue;
    }
    attributes_size++;
    SerializeString(buffer_, bufferIndex, kv.first);
  }

  // dimentions - value
  for (const auto &kv : attributes) {
    if (kv.first.size() > kMaxDimensionNameSize) {
      // warning is already logged earlier, no logging again
      continue;
    }
    if (kv.first == kAttributeAccountKey ||
        kv.first == kAttributeNamespaceKey)
    {
      // custom namespace and account name should't be exported
      continue;
    }
    auto attr_value = AttributeValueToString(kv.second);
    SerializeString(buffer_, bufferIndex, attr_value);
  }

  // two bytes padding for auto-pilot
  SerializeInt<uint16_t>(buffer_, bufferIndex, 0);

  // version - set as 0
  SerializeInt<uint8_t>(buffer_, bufferIndex, 0);

  // Meta-data
  // Value-count pairs is associated with the constant value of 2 in the
  // distribution_type enum.
  SerializeInt<uint8_t>(buffer_, bufferIndex, 2);

  // Keep a position to record how many buckets are added
  auto itemsWrittenIndex = bufferIndex;
  SerializeInt<uint16_t>(buffer_, bufferIndex, 0);

  // bucket values
  size_t index = 0;
  uint16_t bucket_count = 0;
  if (event_type ==
      MetricsEventType::ExternallyAggregatedUlongDistributionMetric) {
    for (auto boundary : boundaries) {
      if (counts[index] > 0) {
        SerializeInt<uint64_t>(buffer_, bufferIndex,
                               static_cast<uint64_t>(boundary));
        SerializeInt<uint32_t>(buffer_, bufferIndex,
                               (uint32_t)(counts[index]));
        bucket_count++;
      }
      index++;
    }
  }

  // write bucket count to previous preserved index
  SerializeInt<uint16_t>(buffer_, itemsWrittenIndex, bucket_count);

  // get final size of payload to be added in front of buffer
  uint16_t body_length = bufferIndex - kBinaryHeaderSize;

  // Add rest of the fields in front of buffer
  bufferIndex = 0;

  // event_type
  SerializeInt<uint16_t>(buffer_, bufferIndex,
                         static_cast<uint16_t>(event_type));

  // body length
  SerializeInt<uint16_t>(buffer_, bufferIndex,
                         static_cast<uint16_t>(body_length));

  // count of dimensions.
  SerializeInt<uint16_t>(buffer_, bufferIndex,
                         static_cast<uint16_t>(attributes_size));

  // reserverd word (2 bytes)
  SerializeInt<uint16_t>(buffer_, bufferIndex, 0);

  // count of events
  SerializeInt<uint32_t>(buffer_, bufferIndex, count);

  // timestamp utc (8 bytes)
  auto windows_ticks = UnixTimeToWindowsTicks(
      std::chrono::duration_cast<std::chrono::duration<std::uint64_t>>(
          ts.time_since_epoch())
          .count());
  SerializeInt<uint64_t>(buffer_, bufferIndex, windows_ticks);

  // sum, min, max

  if (event_type ==
      MetricsEventType::ExternallyAggregatedUlongDistributionMetric) {
    // sum
    SerializeInt<uint64_t>(buffer_, bufferIndex,
                           static_cast<uint64_t>(nostd::get<int64_t>(sum)));
    // min
    SerializeInt<uint64_t>(buffer_, bufferIndex,
                           static_cast<uint64_t>(nostd::get<int64_t>(min)));
    // max
    SerializeInt<uint64_t>(buffer_, bufferIndex,
                           static_cast<uint64_t>(nostd::get<int64_t>(max)));
  } else {
    // won't reach here.
  }
  return body_length;
}

} // namespace metrics
} // namespace geneva
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
