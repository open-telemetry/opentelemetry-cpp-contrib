// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#  include "opentelemetry/metrics/observer_result.h"


class ProcessMetricsFactory
{
public:
    static void GetProcessCpuTime(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetProcessCpuUtilization(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetProcessMemoryUsage(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetProcessMemoryVirtual(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetProcessDiskIO(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetProcessNetworkIO(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetProcessThreads(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetProcessOpenFileDescriptors(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetProcessContextSwitches(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

};