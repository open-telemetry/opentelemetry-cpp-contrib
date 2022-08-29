// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

# include "opentelemetry/exporters/geneva/metrics/exporter.h"
#include "opentelemetry/exporters/geneva/metrics/unix_domain_socket_data_transport.h"
#include "opentelemetry/metrics/meter_provider.h"
#  include "opentelemetry/sdk_config.h"

#include<memory>
#include<mutex>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace geneva
{
namespace metrics
{
    Exporter::Exporter(const ExporterOptions &options):options_(options), 
      connection_string_parser_(options_.connection_string), data_transport_{nullptr},
      buffer_index_histogram_(0), buffer_index_non_histogram_(0)
    {
      if (connection_string_parser_.IsValid()){
        if (connection_string_parser_.transport_protocol_ == TransportProtocol::kUNIX){
          data_transport_ = std::unique_ptr<DataTransport>(new UnixDomainSocketDataTransport(options_.connection_string));
        }
      }
      // Initialize non-histogram buffer
      buffer_index_non_histogram_=  InitializeBufferForNonHistogramData();

      // Initialize histogram buffer
      buffer_index_histogram_= InitiaizeBufferForHistogramData();
    }

    sdk::metrics::AggregationTemporality Exporter::GetAggregationTemporality(
    sdk::metrics::InstrumentType instrument_type) const noexcept
{
  return aggregation_temporality_selector_(instrument_type);
}

opentelemetry::sdk::common::ExportResult Exporter::Export(
    const opentelemetry::sdk::metrics::ResourceMetrics &data) noexcept
{
    auto shutdown = false;
    {
            const std::lock_guard<opentelemetry::common::SpinLockMutex> locked(lock_);
            shutdown = is_shutdown_; 
    }

    if (shutdown)
    {
      OTEL_INTERNAL_LOG_ERROR("[OTLP METRICS gRPC] Exporting "
        << data.scope_metric_data_.size()
        << " metric(s) failed, exporter is shutdown");
      return sdk::common::ExportResult::kFailure;
    }

    if (data.scope_metric_data_.empty())
    {
       return sdk::common::ExportResult::kSuccess;
    }

    for (auto &record : data.scope_metric_data_)
    {
        for (const auto &metric_data:  record.metric_data_)
        {
            for (auto &point_data_with_attributes : metric_data.point_data_attr_)
            if (nostd::holds_alternative<sdk::metrics::SumPointData>(point_data_with_attributes.point_data))
            {
              SerializeNonHistogramMetrics(sdk::metrics::AggregationType::kSum, point_data_with_attributes.point_data, metric_data.end_ts, metric_data.instrument_descriptor.name_, point_data_with_attributes.attributes);
            }
            else if (nostd::holds_alternative<sdk::metrics::LastValuePointData>(point_data_with_attributes.point_data))
            {
              SerializeNonHistogramMetrics(sdk::metrics::AggregationType::kLastValue, point_data_with_attributes.point_data, metric_data.end_ts, metric_data.instrument_descriptor.name_, point_data_with_attributes.attributes);

            } else if (nostd::holds_alternative<sdk::metrics::HistogramPointData>(point_data_with_attributes.point_data))
            {
              SerializeHistogramMetrics(sdk::metrics::AggregationType::kLastValue, point_data_with_attributes.point_data, metric_data.end_ts, metric_data.instrument_descriptor.name_,point_data_with_attributes.attributes);
            }
        }
    }
    return opentelemetry::sdk::common::ExportResult::kSuccess;
}

bool Exporter::ForceFlush(std::chrono::microseconds timeout) noexcept
{
    return true;
}

bool Exporter::Shutdown(std::chrono::microseconds timeout) noexcept
{
  const std::lock_guard<opentelemetry::common::SpinLockMutex> locked(lock_);
  is_shutdown_ = true;
  return true;
}


size_t Exporter::InitializeBufferForNonHistogramData()
{
  // The buffer format is as follows:
  // -- BinaryHeader
  // -- MetricPayload
  // -- Variable length content

  // Leave enough space for the header and fixed payload
  auto bufferIndex =  kBinaryHeaderSize + kMetricPayloadSize;
  SerializeString(buffer_non_histogram_, bufferIndex, connection_string_parser_.account_);
  SerializeString(buffer_non_histogram_, bufferIndex, connection_string_parser_.namespace_);
  return bufferIndex;
}

size_t Exporter::InitiaizeBufferForHistogramData()
{
  // The buffer format is as follows:
  // -- BinaryHeader
  // -- ExternalPayload
  // -- Variable length content

  // Leave enough space for the header and fixed payload
  auto bufferIndex =  kBinaryHeaderSize + kExternalPayloadSize;
  SerializeString(buffer_non_histogram_, bufferIndex, connection_string_parser_.account_);
  SerializeString(buffer_non_histogram_, bufferIndex, connection_string_parser_.namespace_);
  return bufferIndex;
}

size_t Exporter::SerializeNonHistogramMetrics(sdk::metrics::AggregationType agg_type, const sdk::metrics::PointType& point_data,  common::SystemTimestamp ts, std::string metric_name, const sdk::metrics::PointAttributes& attributes)
{
  auto bufferIndex = buffer_index_non_histogram_;
  SerializeString(buffer_non_histogram_, bufferIndex, metric_name);
  for (const auto &kv: attributes ){
    if (kv.first.size() > kMaxDimensionNameSize ) {
      LOG_WARN("Dimension name limit overflow: %s", kv.first);
      continue;
    }
    SerializeString(buffer_non_histogram_, bufferIndex, kv.first);
  }
  for (const auto &kv: attributes){
    auto attr_value = AttributeValueToString(kv.second);
    SerializeString(buffer_non_histogram_, bufferIndex, attr_value);
  }
  // length zero for auto-pilot
  SerializeInt<uint16_t>(buffer_non_histogram_, bufferIndex, 0);

  // get final size of payload
  uint16_t bodyLength = bufferIndex - kBufferSize;

  //Add rest of the fields in front of buffer
  bufferIndex  = k
  SerializeNonHistogramMetrics()







}

}
}
}
OPENTELEMETRY_END_NAMESPACE