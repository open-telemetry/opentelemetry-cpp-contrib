// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

# include "opentelemetry/sdk/metrics/metric_exporter.h"
# include "opentelemetry/exporters/geneva/metrics/exporter_options.h"
#include "opentelemetry/exporters/geneva/metrics/data_transport.h"
#include "opentelemetry/exporters/geneva/metrics/connection_string_parser.h"
# include "opentelemetry/common/spin_lock_mutex.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace geneva
{
namespace metrics
{
/**
 * The Geneva metrics exporter exports metrics data to Geneva 
 */
class Exporter final : public opentelemetry::sdk::metrics::MetricExporter
{
public:

  Exporter(const ExporterOptions &options);

  opentelemetry::sdk::common::ExportResult Export(
      const opentelemetry::sdk::metrics::ResourceMetrics &data) noexcept override;

  sdk::metrics::AggregationTemporality GetAggregationTemporality(
      sdk::metrics::InstrumentType instrument_type) const noexcept override;

  bool ForceFlush(
      std::chrono::microseconds timeout = (std::chrono::microseconds::max)()) noexcept override;

  bool Shutdown(
      std::chrono::microseconds timeout = (std::chrono::microseconds::max)()) noexcept override;

private:
    const ExporterOptions options_;
    ConnectionStringParser connection_string_parser_;
    const sdk::metrics::AggregationTemporalitySelector aggregation_temporality_selector_;
    bool is_shutdown_ = false;
    mutable opentelemetry::common::SpinLockMutex lock_;
    std::unique_ptr<DataTransport> data_transport_;
};
}
}
}
OPENTELEMETRY_END_NAMESPACE