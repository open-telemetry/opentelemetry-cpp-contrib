// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef __linux__

#include "../../include/process_metrics_factory.h"
#include "opentelemetry/nostd/variant.h"
#include "opentelemetry/sdk/metrics/observer_result.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <sys/syscall.h>
#include <linux/perf_event.h>

namespace {

    static void ReadProcessVMandRSS(std::string key, long &mem_size) {
        std::string ret;
        std::ifstream self_status("/proc/self/status");
        std::string line;
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
                            mem_size = std::stol(value_str);
                    }
                }
            }
        }
        mem_size = 0;
    }
}
    void ProcessMetricsFactory::GetProcessCpuTime(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {}

    void ProcessMetricsFactory::GetProcessCpuUtilization(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {}

    void ProcessMetricsFactory::GetProcessMemoryUsage(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {
        long rss_size;
        ReadProcessVMandRSS("VmRSS", rss_size);
        if (rss_size > 0) {
            rss_size = rss_size * 1024 ; //bytes
            opentelemetry::nostd::get<opentelemetry::metrics::ObserverResultT<long>>(observer_result).Observe(rss_size);
        }
    }

    void ProcessMetricsFactory::GetProcessMemoryVirtual(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {
        long vm_size;
        ReadProcessVMandRSS("VmSize", vm_size);
        if (vm_size > 0) {
            vm_size = vm_size * 1024 ; //bytes
            opentelemetry::nostd::get<opentelemetry::metrics::ObserverResultT<long>>(observer_result).Observe(vm_size);
        }
    }

    void ProcessMetricsFactory::GetProcessDiskIO(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {}

    void ProcessMetricsFactory::GetProcessNetworkIO(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {}

    void ProcessMetricsFactory::GetProcessThreads(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {}

    void ProcessMetricsFactory::GetProcessOpenFileDescriptors(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {}

    void ProcessMetricsFactory::GetProcessContextSwitches(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {}

#endif