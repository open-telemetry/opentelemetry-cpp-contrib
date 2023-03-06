#include "opentelemetry/exporters/geneva/metrics/exporter.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/sdk/metrics/aggregation/default_aggregation.h"
#include "opentelemetry/sdk/metrics/aggregation/histogram_aggregation.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h"
#include "opentelemetry/sdk/metrics/meter.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"
#include <memory>
#include <thread>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <dirent.h>

#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <thread>
#include <malloc.h>
#include <time.h>
#include <chrono>
#include <sys/times.h>

namespace metrics_sdk      = opentelemetry::sdk::metrics;
namespace nostd           = opentelemetry::nostd;
namespace common          = opentelemetry::common;
namespace geneva_exporter = opentelemetry::exporter::geneva::metrics;
namespace metrics_api     = opentelemetry::metrics;


namespace {

  void initMetrics(std::string account_name, std::string ns, std::string socket_path, bool is_uds, uint64_t metrics_collection_time_secs) 
  {

  std::cout << " Init metrics : " <<account_name << "-" << ns <<"-"<< socket_path << "-"<< is_uds << "-"<<metrics_collection_time_secs << "\n";
  std::string conn_string = "Account=" + account_name + ";Namespace=" + ns;
  if (!is_uds){
    std::string null_string(1, '\0');
    socket_path  = null_string + socket_path;
  }
  conn_string = "Endpoint=unix://" + socket_path + ";" + conn_string;
  geneva_exporter::ExporterOptions options{conn_string};
  std::unique_ptr<metrics_sdk::PushMetricExporter> exporter{
      new geneva_exporter::Exporter(options)};
    
  std::string version{"1.2.0"};
  std::string schema{"https://opentelemetry.io/schemas/1.2.0"};

  metrics_sdk::PeriodicExportingMetricReaderOptions reader_options;
  reader_options.export_interval_millis = std::chrono::milliseconds(metrics_collection_time_secs *1000);
  reader_options.export_timeout_millis = std::chrono::milliseconds(500);
  std::unique_ptr<metrics_sdk::MetricReader> reader{
      new metrics_sdk::PeriodicExportingMetricReader(std::move(exporter),
                                                    reader_options)};
  auto provider = std::shared_ptr<metrics_api::MeterProvider>(
      new metrics_sdk::MeterProvider());
  auto p = std::static_pointer_cast<metrics_sdk::MeterProvider>(provider);
  p->AddMetricReader(std::move(reader));
  metrics_api::Provider::SetMeterProvider(provider);
}

static void ReadProcSelfFileForKey(std::string file_name, std::string key, long &value) {
    std::string ret;
    std::ifstream self_status(file_name, std::ifstream::in);
    std::string line;
    bool found = false;
    while (std::getline(self_status, line)) 
    {
        std::istringstream is_line(line);
        std::string   field0;
        if(std::getline(is_line, field0, ':')) 
        {
            if (field0 == key)
            {
                std::string value_str;
                if(std::getline(is_line, value_str)) 
                {
                    value_str.erase(std::remove_if(value_str.begin(), value_str.end(),
                        []( auto const& c ) -> bool { return not std::isdigit(c); } ), value_str.end());
                        value = std::stol(value_str);
                        found = true;
                }
            }
        }
    }
    value = found ? value : -1;
    self_status.close();
}

static void ReadNetworkIOStats(long &total_read_bytes, long &total_write_bytes) {
    std::string ret;
	const unsigned initial_headers_idx = 2; // to be ignored
	const unsigned read_idx = 1; // column number containing received bytes
	const unsigned write_idx = 9; // column number containing sent bytes
    const std::string loop_back_interface = "lo:";
	unsigned line_idx = 0;
    std::ifstream self_status("/proc/self/net/dev", std::ifstream::in);
    std::string line;
    while (std::getline(self_status, line)) 
    {
        if (line_idx++ < initial_headers_idx) {
		    continue;
	    }
        std::stringstream  lineStream(line);
        std::string value;
        size_t count = 0;
        while ( lineStream >> value){
            if (count > write_idx){
                break;
            }
            if (count == 0 && value == loop_back_interface){
                break;
            }
            if (count == read_idx){
                total_read_bytes += std::stol(value);
            }
            if (count == write_idx){
                total_write_bytes += std::stol(value);
            }
            count++;
        }
    }
}

void GetProcessCpuTime(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/) 
{
    static long clock_ticks_per_sec_ = sysconf(_SC_CLK_TCK);
    struct tms current_cpu_time;
    times(&current_cpu_time);
    long cpu_time = (current_cpu_time.tms_utime + current_cpu_time.tms_stime)/clock_ticks_per_sec_;
    nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(cpu_time);
}

void GetProcessMemoryUsage(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
{
    long rss_bytes = 0;
    ReadProcSelfFileForKey("/proc/self/status", "VmRSS", rss_bytes);
    if (rss_bytes >= 0) {
        rss_bytes = rss_bytes * 1024 ; //bytes
        nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(rss_bytes);
    }  
}

void GetProcessMemoryVirtual(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
{
    long vm_bytes = 0;
    ReadProcSelfFileForKey("/proc/self/status", "VmSize", vm_bytes);
    if (vm_bytes >= 0) {
        vm_bytes = vm_bytes * 1024 ; //bytes
        nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(vm_bytes);
    }   
}

void GetProcessDiskIO(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
{
    long read_bytes = 0, write_bytes = 0;
    ReadProcSelfFileForKey("/proc/self/io", "read_bytes", read_bytes);
    if (read_bytes >= 0 ){
        nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(read_bytes, {{"direction", "read"}});
    }
    ReadProcSelfFileForKey("/proc/self/io", "write_bytes", write_bytes);
    if (write_bytes >= 0 ){
        nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(write_bytes, {{"direction", "write"}});
    }
}

void GetProcessNetworkIO(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
{
    long read_bytes = 0, write_bytes = 0;
    ReadNetworkIOStats(read_bytes, write_bytes);
    if (read_bytes > 0 ) {
        nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(read_bytes, {{"direction", "receive"}});
    }
    if (write_bytes > 0){
        nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(write_bytes, {{"direction", "transmit"}});
    }  
}

void GetProcessThreads(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
{
    long threads_count = 0;
    ReadProcSelfFileForKey("/proc/self/status", "Threads", threads_count);
    if (threads_count > 0){
        opentelemetry::nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(threads_count);
    }
}

void GetProcessOpenFileDescriptors(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
{
    std::string path = "/proc/self/fd/";
    auto dir = opendir(path.data());
    size_t count_fds = 0;
    while (auto f = readdir(dir)) {
        if (!f->d_name || f->d_name[0] == '.')
        {
            continue ; //Skip everything that starts with a dot
        }
        count_fds ++;
    }
    closedir(dir);
    opentelemetry::nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(count_fds);
}

void GetProcessHeapMemory(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
{
    struct mallinfo m = mallinfo();
    long used = m.uordblks  + m.hblkhd ;
    opentelemetry::nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(used, {{"type", "memory_usage_mallinfo"}});
}

void GetProcessContextSwitches(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
{
    long voluntary_ctxt_switches = 0;
    long nonvoluntary_ctxt_switches = 0;
    ReadProcSelfFileForKey("/proc/self/status", "voluntary_ctxt_switches", voluntary_ctxt_switches);
    ReadProcSelfFileForKey("/proc/self/status", "nonvoluntary_ctxt_switches", nonvoluntary_ctxt_switches);
    auto total_ctxt_switches = voluntary_ctxt_switches + nonvoluntary_ctxt_switches;
    opentelemetry::nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(voluntary_ctxt_switches, {{"type", "voluntary_ctxt_switches"}});
    opentelemetry::nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(nonvoluntary_ctxt_switches, {{"type", "nonvoluntary_ctxt_switches"}});
}

void create_process_cpu_time_observable_gauge()
{
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> cpu_time_obserable_gauge_;
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0", "schema");
  cpu_time_obserable_gauge_ = meter->CreateInt64ObservableGauge("process.cpu.time", "des", "unit");
  cpu_time_obserable_gauge_->AddCallback(GetProcessCpuTime, nullptr);
}

void create_process_memory_usage_observable_gauge()
{
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> memory_usage_obserable_gauge_;
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  memory_usage_obserable_gauge_ = meter->CreateInt64ObservableGauge("process.memory.physical", "des", "unit");
  memory_usage_obserable_gauge_->AddCallback(GetProcessMemoryUsage, nullptr);  
}

void create_process_memory_virtual_observable_gauge()
{
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> memory_virtual_obserable_gauge_;
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  memory_virtual_obserable_gauge_ = meter->CreateInt64ObservableGauge("process.memory.virtual", "des", "unit");
  memory_virtual_obserable_gauge_->AddCallback(GetProcessMemoryVirtual, nullptr);   
}

void create_process_heap_memory_observable_gauge()
{
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> memory_heap_obserable_gauge_; 
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  memory_heap_obserable_gauge_ = meter->CreateInt64ObservableGauge("process.memory.heap", "des", "unit");
  memory_heap_obserable_gauge_->AddCallback(GetProcessHeapMemory, nullptr);   
}

void create_process_disk_io_observable_gauge()
{
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> disk_io_obserable_gauge_;
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  disk_io_obserable_gauge_ = meter->CreateInt64ObservableGauge("process.disk.io", "des", "unit");
  disk_io_obserable_gauge_->AddCallback(GetProcessDiskIO, nullptr);
}

void create_process_network_io_observable_gauge()
{
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> network_io_obserable_gauge_;
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  network_io_obserable_gauge_ = meter->CreateInt64ObservableGauge("process.network.io", "des", "unit");
  network_io_obserable_gauge_->AddCallback(GetProcessNetworkIO, nullptr);   
}

void create_process_threads_observable_gauge()
{
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> threads_obserable_gauge_;
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  threads_obserable_gauge_ = meter->CreateInt64ObservableGauge("process.threads", "des", "unit");
  threads_obserable_gauge_->AddCallback(GetProcessThreads, nullptr);
}

void create_process_open_files_observable_gauge()
{
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> open_files_obserable_gauge_;
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  open_files_obserable_gauge_ = meter->CreateInt64ObservableGauge("process.open.files", "des", "unit");
  open_files_obserable_gauge_->AddCallback(GetProcessOpenFileDescriptors, nullptr);
}

void create_process_context_switches_observable_gauge()
{
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> context_switches_obserable_gauge_;
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
  context_switches_obserable_gauge_ = meter->CreateInt64ObservableGauge("process.context.switches", "des", "unit");
  context_switches_obserable_gauge_->AddCallback(GetProcessContextSwitches, nullptr);
}

void start_stress_test_counter(size_t dimension_count, size_t dimension_cardinality, size_t delay_between_measurements_secs, size_t number_of_threads, std::vector<std::thread> &measurementThreads)
{
    measurementThreads.resize(0);
    auto provider = metrics_api::Provider::GetMeterProvider();
    nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
    static opentelemetry::nostd::unique_ptr<opentelemetry::metrics::Counter<double>> instrument = meter->CreateDoubleCounter("counter1", "counter1_description", "counter1_unit");
    for (int i = 0; i < number_of_threads; i++){
        measurementThreads.push_back(std::thread( [i , dimension_count, dimension_cardinality, delay_between_measurements_secs]() {
            while(true)
            {
                std::map<std::string, size_t> properties;
                for (int j = 0 ; j < dimension_count ; j++){
                    std::string key = "dimension" +  std::to_string(i);
                    size_t rand_val = rand() % dimension_cardinality;
                    properties[key] = rand_val;
                }
                size_t rand1 = rand() % 10;
                size_t rand2 = rand() % 10;
                size_t rand3 = rand() % 10;
                instrument->Add(1.0, properties);
            }
        }));
    }
}

void start_stress_test_histogram(size_t dimension_count, size_t dimension_cardinality, size_t delay_between_measurements_secs, size_t number_of_threads, std::vector<std::thread> &measurementThreads)
{
    auto provider = metrics_api::Provider::GetMeterProvider();
    nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("process.metrics", "1.2.0");
    auto instrument = meter->CreateDoubleHistogram("histogram1", "histogram1_description", "histogram1_unit");
    for (int i = 0; i < number_of_threads; i++){
        measurementThreads.push_back(std::thread( [&instrument, i , &dimension_count, &dimension_cardinality, &delay_between_measurements_secs]() {
            while(true)
            {
                std::map<std::string, size_t> properties;
                for (int i = 0 ; i < dimension_count ; i++){
                    std::string key = "dimension" +  std::to_string(i);
                    size_t rand_val = rand() % dimension_cardinality;
                    properties[key] = rand_val;
                }
                size_t rand1 = rand() % 10;
                size_t rand2 = rand() % 10;
                size_t rand3 = rand() % 10;
                instrument->Record(1.0, properties);
                std::this_thread::sleep_for(std::chrono::seconds(delay_between_measurements_secs));
            }
        }));
    }
}
}

int main(int argc, char **argv)
{
  std::vector<std::string> args(argv + 1, argv + argc);
  std::string example_type;
  if (args.size() == 0 || args[0] == "--help") {
      std::cout <<  "Options -- \n" ;
      std::cout << "\t --geneva_account_name [default=test_account] --geneva_namespace [default=test_ns]\n";
      std::cout << "\t --socket_type [uds|abstract, default=uds] --socket_path [path default=/tmp/geneva.socket]\n";
      std::cout << "\t --metrics_collection_time_secs [default=60]\n";
      std::cout <<  "\t --process.cpu.time --process.cpu.utilization --process.memory.physical --process.memory.virtual --process.memory.heap\n" ;
      std::cout <<  "\t --process.disk.io --process.network.io --process.threads --process.open.files --process.context.switches --stress.test\n";
      std::cout << "\t --dimension_count [default=3]  --dimension_cardinality [default=5] --delay_between_measurements_secs [default=0]\n"; // relevant options for stress test
      std::cout << "\t --measurement_type [counter|histogram default=counter] --number_of_threads [default=1]\n"; // relevant options for stress test
      std::cout << "\n";
      exit(1); 
  }

  // read initial config options
  size_t index = 0;
  std::string account_name = "test_account";
  std::string geneva_ns = "test_ns";
  std::string socket_path = "/tmp/geneva.socket";
  bool is_uds = true;
  bool is_stress_test = false;
  uint64_t collection_ts = 60;
  while (index < args.size())
  {
    if (args[index] == "--geneva_account_name")
    {
        account_name = args[++index];
    }
    else if (args[index] == "--geneva_namespace")
    {
        geneva_ns = args[++index];
    }
    else if (args[index] == "--socket_type")
    {
        if (args[++index] == "abstract"){
            is_uds = false;
        }
    }
    else if (args[index] == "--socket_path")
    {
        socket_path = args[++index];
    }
    else if (args[index] == "--metrics_collection_time_secs")
    {
        collection_ts = atoi(args[++index].c_str());
    }
    else if (args[index] == "--stress.test")
    {
        is_stress_test = true;
    }
    index++;
  }

   // read measurement_type, dimension_count, dimension_cardinality, delay_between_measurements_secs
   size_t dimension_count = 3;
   size_t dimension_cardinality = 5;
   size_t delay_between_measurements_secs = 0;
   size_t number_of_threads = 1;
   std::string measurement_type = "counter";
  
   if (is_stress_test) 
   {
    index = 0;
    while(index < args.size())
    {
        if(args[index] == "--dimension_count")
        {
            dimension_count  = atoi(args[++index].c_str());
        }
        else if (args[index] == "--dimension_cardinality")
        {
            dimension_cardinality = atoi(args[++index].c_str());
        }
        else if (args[index] == "--measurement_type")
        {
            measurement_type = args[++index];
        }
        else if (args[index] == "--delay_between_measurements_secs")
        {
            delay_between_measurements_secs =  atoi(args[++index].c_str());
        }
        else if (args[index] == "--number_of_threads")
        {
            number_of_threads =  atoi(args[++index].c_str());
        }
        index++;
    }
   }

  initMetrics(account_name, geneva_ns, socket_path, is_uds, collection_ts);


  // read rest of the arguments.
  for (auto &arg: args) {
    if (arg == "--process.cpu.time") {
      create_process_cpu_time_observable_gauge();
    }
    else if (arg == "--process.memory.physical")
    {
      create_process_memory_usage_observable_gauge();
    }
    else if (arg == "--process.memory.virtual")
    {
      create_process_memory_virtual_observable_gauge();
    }
    else if (arg == "--process.memory.heap")
    {
      create_process_heap_memory_observable_gauge();
    }
    else if (arg == "--process.disk.io")
    {
      create_process_disk_io_observable_gauge();
    }
    else if (arg == "--process.network.io")
    {
      create_process_network_io_observable_gauge();
    }
    else if (arg == "--process.threads")
    {
      create_process_threads_observable_gauge();
    }
    else if(arg == "--process.open.files")
    {
      create_process_open_files_observable_gauge();
    }
    else if (arg == "--process.context.switches")
    {
      create_process_context_switches_observable_gauge();
    }
  }

  std::vector<std::thread> measurementThreads;

  if (is_stress_test)
  {
    if (measurement_type == "counter")
    {
        start_stress_test_counter(dimension_count, dimension_cardinality, delay_between_measurements_secs, number_of_threads, measurementThreads);
    }
    else if (measurement_type == "histogram") 
    {
        start_stress_test_histogram(dimension_count, dimension_cardinality, delay_between_measurements_secs, number_of_threads, measurementThreads);
    }
  }

  while(true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  for (auto &thread: measurementThreads){
    thread.join();
  }
}
