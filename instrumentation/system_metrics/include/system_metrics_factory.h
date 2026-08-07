// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#  include "opentelemetry/metrics/observer_result.h"

class SystemMetricsFactory
{
public:
    static void GetSystemCpuTime(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetSystemCpuUtilization(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetSystemMemoryUsage(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetSystemMemoryUtilization(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetSystemSwapUsage(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetSystemSwapUtilization(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetSystemDiskIO(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetSystemDiskOperations(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetSystemDiskTime(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

    static void GetSystemDiskMerged(opentelemetry::metrics::ObserverResult observer_result, void * /*state*/);

};