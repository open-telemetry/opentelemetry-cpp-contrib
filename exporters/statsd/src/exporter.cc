// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/statsd/metrics/exporter.h"
#include "opentelemetry/exporters/statsd/metrics/macros.h"
#ifdef _WIN32
#include "opentelemetry/exporters/statsd/metrics/etw_data_transport.h"
#else
#include "opentelemetry/exporters/statsd/metrics/socket_data_transport.h"
#endif
#include "opentelemetry/sdk/metrics/export/metric_producer.h"
#include "opentelemetry/sdk_config.h"

#include <memory>
#include <mutex>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace statsd {
namespace metrics {
Exporter::Exporter(const ExporterOptions &options)
    : options_(options), connection_string_parser_(options_.connection_string),
      data_transport_{nullptr} {
  if (connection_string_parser_.IsValid()) {
#ifdef _WIN32
    if (connection_string_parser_.transport_protocol_ ==
             TransportProtocol::kETW) {
      data_transport_ = std::unique_ptr<DataTransport>(
          new ETWDataTransport(kBinaryHeaderSize));
    }
#else
    if (connection_string_parser_.transport_protocol_ == TransportProtocol::kUNIX
      || connection_string_parser_.transport_protocol_ == TransportProtocol::kTCP
      || connection_string_parser_.transport_protocol_ == TransportProtocol::kUDP) {
      data_transport_ =
          std::unique_ptr<DataTransport>(new SocketDataTransport(
              connection_string_parser_));
    }
#endif
  }
  // Connect transport at initialization
  auto status = data_transport_->Connect();
  if (!status) {
    LOG_ERROR("[Statsd Exporter] Connect failed. No data would be sent.");
    is_shutdown_ = true;
    return;
  }
}

sdk::metrics::AggregationTemporality Exporter::GetAggregationTemporality(
    sdk::metrics::InstrumentType instrument_type) const noexcept {
  if (instrument_type == sdk::metrics::InstrumentType::kUpDownCounter)
    {
      return sdk::metrics::AggregationTemporality::kCumulative;
    }
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
    OTEL_INTERNAL_LOG_ERROR("[Statsd Exporter] Exporting "
                            << data.scope_metric_data_.size()
                            << " metric(s) failed, exporter is shutdown");
    return sdk::common::ExportResult::kFailure;
  }

  if (data.scope_metric_data_.empty()) {
    return sdk::common::ExportResult::kSuccess;
  }

  for (auto &record : data.scope_metric_data_) {
    for (const auto &metric_data : record.metric_data_) {
      MetricsEventType event_type;
      if (metric_data.instrument_descriptor.type_ ==
          sdk::metrics::InstrumentType::kUpDownCounter) {
        event_type = MetricsEventType::Gauge;
      } else if (metric_data.instrument_descriptor.type_ ==
                 sdk::metrics::InstrumentType::kObservableCounter) {
        event_type = MetricsEventType::ScaledFixedPointNumber;
      } else if (metric_data.instrument_descriptor.type_ ==
                 sdk::metrics::InstrumentType::kObservableGauge) {
        event_type = MetricsEventType::FloatingPointNumber;
      } else {
        event_type = MetricsEventType::Unknown;
      }
      if (event_type == MetricsEventType::Unknown) {
        LOG_ERROR("[Statsd Exporter] Exporting "
                  << metric_data.instrument_descriptor.name_
                  << " failed, unsupported metric type");
        continue;
      }
      for (auto &point_data_with_attributes : metric_data.point_data_attr_) {
        ValueType new_value;
        if (nostd::holds_alternative<sdk::metrics::SumPointData>(
                point_data_with_attributes.point_data)) {
          auto value = nostd::get<sdk::metrics::SumPointData>(
              point_data_with_attributes.point_data);
          new_value = value.value_;

  
          if (!nostd::holds_alternative<double>(value.value_) && !value.is_monotonic_) {
            // NOTE - Potential for minor precision loss implicitly going from
            // int64_t to double -
            //   - A 64-bit integer can hold more significant decimal digits
            //   than a standard
            //     IEEE (64-bit) double precision floating-point
            //     representation
            new_value = static_cast<double>(nostd::get<int64_t>(new_value));
          }

        } else if (nostd::holds_alternative<sdk::metrics::LastValuePointData>(
                       point_data_with_attributes.point_data)) {
          auto value = nostd::get<sdk::metrics::LastValuePointData>(
              point_data_with_attributes.point_data);
          new_value = value.value_;
          if (nostd::holds_alternative<int64_t>(value.value_)) {
            // NOTE - Potential for minor precision loss implicitly going from
            // int64_t to double -
            //   - A 64-bit integer can hold more significant decimal digits
            //   than a standard
            //     IEEE (64-bit) double precision floating-point representation
            new_value = static_cast<double>(nostd::get<int64_t>(new_value));
          }
          
        } else if (nostd::holds_alternative<sdk::metrics::HistogramPointData>(
                       point_data_with_attributes.point_data)) {
          auto value = nostd::get<sdk::metrics::HistogramPointData>(
              point_data_with_attributes.point_data);
          ValueType new_sum = value.sum_;
          ValueType new_min = value.min_;
          ValueType new_max = value.max_;
        }

        this->SendMetrics(metric_data.instrument_descriptor.name_,
                    event_type, new_value);
      }
    }
  }
  return opentelemetry::sdk::common::ExportResult::kSuccess;
}

void Exporter::SendMetrics(std::string metric_name, MetricsEventType type, ValueType value) noexcept {
  std::string message;

  std::string value_str;
  if (nostd::holds_alternative<int64_t>(value)) {
    value_str = std::to_string(nostd::get<int64_t>(value));
  } else if (nostd::holds_alternative<double>(value)) {
    value_str = std::to_string(nostd::get<double>(value));
  } else {
    LOG_ERROR("[Statsd Exporter] SendMetrics - "
              "Unsupported value type");
    return;
  }

  message = metric_name + ":" + value_str + "|" +
            MetricsEventTypeToString(type);
  data_transport_->Send(type, message.c_str(), message.size());
}

bool Exporter::ForceFlush(std::chrono::microseconds timeout) noexcept {
  return true;
}

bool Exporter::Shutdown(std::chrono::microseconds timeout) noexcept {
  const std::lock_guard<opentelemetry::common::SpinLockMutex> locked(lock_);
  is_shutdown_ = true;
  return true;
}

} // namespace metrics
} // namespace statsd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
