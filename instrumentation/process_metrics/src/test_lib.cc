#include "../include/test_lib.h"
#include "../include/process_metrics_factory.h"
#  include "opentelemetry/context/context.h"
#  include "opentelemetry/metrics/provider.h"

namespace nostd       = opentelemetry::nostd;
namespace metrics_api = opentelemetry::metrics;

void TestLib::create_process_cpu_time_observable_counter()
{
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  auto obserable_counter = meter->CreateLongObservableCounter("process.cpu.time", "des", "unit");
  obserable_counter->AddCallback(ProcessMetricsFactory::GetProcessCpuTime, nullptr);
}

void TestLib::create_process_cpu_utilization_observable_gauge()
{
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  auto obserable_gauge = meter->CreateLongObservableGauge("process.cpu.utilization", "des", "unit");
  obserable_gauge->AddCallback(ProcessMetricsFactory::GetProcessCpuUtilization, nullptr);
}

void TestLib::create_process_memory_usage_observable_gauge()
{
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  auto obserable_gauge = meter->CreateLongObservableGauge("process.memory.usage", "des", "unit");
  obserable_gauge->AddCallback(ProcessMetricsFactory::GetProcessMemoryUsage, nullptr);
}

void TestLib::create_process_memory_virtual_observable_gauge()
{
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  auto obserable_gauge = meter->CreateLongObservableGauge("process.memory.virtual", "des", "unit");
  obserable_gauge->AddCallback(ProcessMetricsFactory::GetProcessMemoryVirtual, nullptr);
}

void TestLib::create_process_disk_io_observable_counter()
{
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  auto obserable_gauge = meter->CreateLongObservableGauge("process.disk.io", "des", "unit");
  obserable_gauge->AddCallback(ProcessMetricsFactory::GetProcessDiskIO, nullptr);
}

void TestLib::create_process_network_io_observable_counter()
{
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  auto obserable_gauge = meter->CreateLongObservableGauge("process.network.io", "des", "unit");
  obserable_gauge->AddCallback(ProcessMetricsFactory::GetProcessNetworkIO, nullptr);   
}

void TestLib::create_process_threads_observable_counter()
{
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  auto obserable_gauge = meter->CreateLongObservableGauge("process.threads", "des", "unit");
  obserable_gauge->AddCallback(ProcessMetricsFactory::GetProcessThreads, nullptr);
}

void TestLib::create_process_open_files_observable_gauge()
{
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  auto obserable_gauge = meter->CreateLongObservableGauge("process.open.files", "des", "unit");
  obserable_gauge->AddCallback(ProcessMetricsFactory::GetProcessOpenFileDescriptors, nullptr);
}

void TestLib::create_process_context_switches_observable_counter()
{
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  auto obserable_gauge = meter->CreateLongObservableGauge("process.context.switches", "des", "unit");
  obserable_gauge->AddCallback(ProcessMetricsFactory::GetProcessContextSwitches, nullptr);   

}
