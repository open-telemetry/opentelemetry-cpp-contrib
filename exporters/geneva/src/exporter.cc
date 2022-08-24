// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef ENABLE_METRICS_PREVIEW

# include "opentelemetry/exporters/geneva/metrics/exporter.h"
#  include "opentelemetry/sdk_config.h"


#include<mutex>
OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace geneva
{
namespace metrics
{
    Exporter::Exporter(const ExporterOptions &options):options_(options), 
      connection_string_parser_(options_.connection_string)
    {
      if (connection_string_parser_.IsValid()){
        if (connection_string_parser_.transport_protocol_ == k)
      }
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
        for (const auto &point_data:  record.metric_data_)
        {
            if (nostd::holds_alternative<sdk::metrics::SumPointData>(point_data))
            {
                
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

}
}
}
OPENTELEMETRY_END_NAMESPACE