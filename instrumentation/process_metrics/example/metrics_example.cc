#  include <memory>
#  include <thread>
#  include "opentelemetry/exporters/ostream/metric_exporter.h"
#  include "opentelemetry/metrics/provider.h"
#  include "opentelemetry/sdk/metrics/aggregation/default_aggregation.h"
#  include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h"
#  include "opentelemetry/sdk/metrics/meter.h"
#  include "opentelemetry/sdk/metrics/meter_provider.h"

#include "../include/test_lib.h"

namespace metric_sdk      = opentelemetry::sdk::metrics;
namespace nostd           = opentelemetry::nostd;
namespace common          = opentelemetry::common;
namespace exportermetrics = opentelemetry::exporter::metrics;
namespace metrics_api     = opentelemetry::metrics;

namespace
{

void initMetrics()
{
  std::unique_ptr<metric_sdk::MetricExporter> exporter{new exportermetrics::OStreamMetricExporter};

  std::string version{"1.2.0"};
  std::string schema{"https://opentelemetry.io/schemas/1.2.0"};

  // Initialize and set the global MeterProvider
  metric_sdk::PeriodicExportingMetricReaderOptions options;
  options.export_interval_millis = std::chrono::milliseconds(1000);
  options.export_timeout_millis  = std::chrono::milliseconds(500);
  std::unique_ptr<metric_sdk::MetricReader> reader{
      new metric_sdk::PeriodicExportingMetricReader(std::move(exporter), options)};
  auto provider = std::shared_ptr<metrics_api::MeterProvider>(new metric_sdk::MeterProvider());
  auto p        = std::static_pointer_cast<metric_sdk::MeterProvider>(provider);
  p->AddMetricReader(std::move(reader));

  metrics_api::Provider::SetMeterProvider(provider);
}
}  // namespace

int main(int argc, char **argv)
{
    initMetrics();
  std::string example_type;
  if (argc >= 2)
  {
    example_type = argv[1];
  }
  if (example_type == "process.cpu.time")
  {
    TestLib::create_process_cpu_time_observable_counter();
  }
  while(true){
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  }




