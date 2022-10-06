// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef __linux__

#include "../../include/process_metrics_factory.h"
#include "opentelemetry/nostd/variant.h"
#include "opentelemetry/sdk/metrics/observer_result.h"
#include "process_cpu_time.h"

#include <stdio.h>
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

using namespace opentelemetry;

namespace {

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
}
    void ProcessMetricsFactory::GetProcessCpuTime(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {
        static ProcessCpuTime cputime;
        long system_time = 0, user_time = 0;
        cputime.TotalElapsedSystemAndUserTime(system_time, user_time);
        nostd::get<nostd::shared_ptr<metrics::ObserverResultT<long>>>(observer_result)->Observe(system_time, {{"state", "system"}});
        nostd::get<nostd::shared_ptr<metrics::ObserverResultT<long>>>(observer_result)->Observe(user_time, {{"state", "user"}});
    }

    void ProcessMetricsFactory::GetProcessCpuUtilization(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {
        //TODO
    }

    void ProcessMetricsFactory::GetProcessMemoryUsage(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {
        long rss_bytes = 0;
        ReadProcSelfFileForKey("/proc/self/status", "VmRSS", rss_bytes);
        if (rss_bytes >= 0) {
            rss_bytes = rss_bytes * 1024 ; //bytes
            nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(rss_bytes);
        }
    }

    void ProcessMetricsFactory::GetProcessMemoryVirtual(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {
        long vm_bytes = 0;
        ReadProcSelfFileForKey("/proc/self/status", "VmSize", vm_bytes);
        if (vm_bytes >= 0) {
            vm_bytes = vm_bytes * 1024 ; //bytes
            nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(vm_bytes);
        }
    }

    void ProcessMetricsFactory::GetProcessDiskIO(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
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

    void ProcessMetricsFactory::GetProcessNetworkIO(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
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

    void ProcessMetricsFactory::GetProcessThreads(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {
        long threads_count = 0;
        ReadProcSelfFileForKey("/proc/self/status", "Threads", threads_count);
        if (threads_count > 0){
            opentelemetry::nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result)->Observe(threads_count);
        }
    }

    void ProcessMetricsFactory::GetProcessOpenFileDescriptors(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
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

    void ProcessMetricsFactory::GetProcessContextSwitches(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {}




#endif
