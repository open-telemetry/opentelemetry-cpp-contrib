// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef __linux__
#include "process_cpu_time.h"
#include <unistd.h>
#include <thread>
#include <iostream>
#include <math.h>


long ProcessCpuTime::clock_ticks_per_sec_ = sysconf(_SC_CLK_TCK);

ProcessCpuTime::ProcessCpuTime()
: number_of_cpus_{std::thread::hardware_concurrency()}, last_clock_time_{std::chrono::high_resolution_clock::now()}
{
    times(&start_time_);
    last_cpu_time_ = start_time_;
}

long ProcessCpuTime::TotalElapsedTime()
{
    times(&last_cpu_time_);
    return ((last_cpu_time_.tms_utime + last_cpu_time_.tms_stime)  - (start_time_.tms_utime + start_time_.tms_stime)) / clock_ticks_per_sec_;
}

double ProcessCpuTime::LastElapsedTime()
{
    struct tms current_cpu_time;
    times(&current_cpu_time);
    auto elapsed_cpu_time = ((current_cpu_time.tms_utime + current_cpu_time.tms_stime)  - (last_cpu_time_.tms_utime + last_cpu_time_.tms_stime)) / clock_ticks_per_sec_;
    last_cpu_time_ = current_cpu_time;
    return elapsed_cpu_time;
}

double ProcessCpuTime::CpuUtilization() 
{
    struct tms current_cpu_time;
    times(&current_cpu_time);
    auto current_clock_time = std::chrono::high_resolution_clock::now();
    auto elapsed_cpu_time = ((current_cpu_time.tms_utime + current_cpu_time.tms_stime)  - (last_cpu_time_.tms_utime + last_cpu_time_.tms_stime)) / clock_ticks_per_sec_;
    auto elapsed_clock_time = std::chrono::duration_cast<std::chrono::seconds>(current_clock_time - last_clock_time_);
    last_clock_time_ = current_clock_time;
    double cpu_utlization = 0;
    if (elapsed_clock_time.count() > 0)
        cpu_utlization = elapsed_cpu_time/(elapsed_clock_time.count() * number_of_cpus_);
    return cpu_utlization;
}

#endif