// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef __linux__
#include "process_cpu_time.h"
#include <unistd.h>
#include <thread>
#include <iostream>


long ProcessCpuTime::clock_ticks_per_sec_ = sysconf(_SC_CLK_TCK);

ProcessCpuTime::ProcessCpuTime()
: number_of_cpus_{std::thread::hardware_concurrency()}
{
    times(&start_time_);
    last_time_ = start_time_;

}

long ProcessCpuTime::TotalElapsedTime()
{
    times(&last_time_);
    return ((last_time_.tms_utime + last_time_.tms_stime)  - (start_time_.tms_utime + start_time_.tms_stime)) / clock_ticks_per_sec_;
}

void ProcessCpuTime::TotalElapsedSystemAndUserTime(long &system_time, long &user_time)
{
    times(&last_time_);
    std::cout << " ===> " << last_time_.tms_utime << " == " << last_time_.tms_stime << " == " << clock_ticks_per_sec_ << "\n";
    user_time = (last_time_.tms_utime - start_time_.tms_utime ) / clock_ticks_per_sec_;
    system_time = (last_time_.tms_stime - start_time_.tms_stime ) / clock_ticks_per_sec_;
}

double ProcessCpuTime::LastElapsedTime()
{
    struct tms current_time_;
    times(&current_time_);
    auto elapsed_total_time = ((current_time_.tms_utime + current_time_.tms_stime)  - (last_time_.tms_utime + last_time_.tms_stime)) / clock_ticks_per_sec_;
    last_time_ = current_time_;
    return elapsed_total_time;
}

double ProcessCpuTime::CpuUtilization() 
{
    struct tms current_time_;
    times(&current_time_);
    auto elapsed_total_time = ((current_time_.tms_utime + current_time_.tms_stime)  - (last_time_.tms_utime + last_time_.tms_stime)) / clock_ticks_per_sec_;
    auto elapsed_cpu_time = ( current_time_.tms_stime  - last_time_.tms_stime) / clock_ticks_per_sec_;
    last_time_ = current_time_;
    return elapsed_cpu_time/(elapsed_total_time * number_of_cpus_);
}

void ProcessCpuTime::LastElapsedSystemAndUserTime(long &system_time, long &user_time)
{
    struct tms current_time_;
    times(&current_time_);
    std::cout << " ===> " << current_time_.tms_utime << " == " << current_time_.tms_stime <<   " == " << clock_ticks_per_sec_ << "\n";
    user_time = (current_time_.tms_utime - last_time_.tms_utime) / clock_ticks_per_sec_;
    system_time = (current_time_.tms_stime - last_time_.tms_stime) / clock_ticks_per_sec_;
    last_time_ = current_time_;
}

#endif