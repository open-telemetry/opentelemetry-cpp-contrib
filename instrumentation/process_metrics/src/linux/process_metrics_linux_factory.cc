// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef __linux__

#include "../../include/process_metrics_factory.h"

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


    void ProcessMetricsFactory::GetProcessCpuTime(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {}

    void ProcessMetricsFactory::GetProcessCpuUtilization(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {}

    void ProcessMetricsFactory::GetProcessMemoryUsage(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {}

    void ProcessMetricsFactory::GetProcessMemoryVirtual(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/)
    {}

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