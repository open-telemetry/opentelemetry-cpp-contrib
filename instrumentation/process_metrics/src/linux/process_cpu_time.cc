// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef __linux__
#include "process_cpu_time.h"
#include <unistd.h>

long ProcessCpuTime::clock_ticks_per_sec_ = sysconf(_SC_CLK_TCK);

ProcessCpuTime::ProcessCpuTime()
{
    times(&start_time_);
    last_time_ = start_time_;
}

double ProcessCpuTime::TotalElapsedTime()
{
    times(&last_time_);
    return ((last_time_.tms_utime + last_time_.tms_stime)  - (start_time_.tms_utime + start_time_.tms_stime)) / clock_ticks_per_sec_;
}

double ProcessCpuTime::TotalElapsedSystemAndUserTime(double &system_time, double &user_time)
{
    times(&last_time_);
    user_time = (last_time_.tms_utime - start_time_.tms_utime ) / clock_ticks_per_sec_;
    system_time = (last_time_.tms_stime - start_time_.tms_stime ) / clock_ticks_per_sec_;
}

double ProcessCpuTime::LastElapsedTime()
{
    struct tms current_time_;
    times(&current_time_);
    auto elapsed_time = ((current_time_.tms_utime + current_time_.tms_stime)  - (last_time_.tms_utime + last_time_.tms_stime)) / clock_ticks_per_sec_;
    last_time_ = current_time_;
    return elapsed_time;
}

double ProcessCpuTime::LastElapsedSystemAndUserTime(double &system_time, double &user_time)
{
    struct tms current_time_;
    times(&current_time_);
    user_time = (current_time_.tms_utime - last_time_.tms_utime) / clock_ticks_per_sec_;
    system_time = (current_time_.tms_stime - last_time_.tms_stime) / clock_ticks_per_sec_;
    last_time_ = current_time_;
}

#endif