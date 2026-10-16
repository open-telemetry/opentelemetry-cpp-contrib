// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef __linux__
#include <time.h>
#include <chrono>
#include <sys/times.h>

class ProcessCpuTime
{
    public:
    ProcessCpuTime();

    // returns cpu time (user + system) from beginning
    long TotalElapsedTime();

    // returns cpu time (user + system) since last call to Last*Time()
    double LastElapsedTime();

    // returns cpu utilization
    double CpuUtilization();

    private:
        static long clock_ticks_per_sec_;
        struct tms start_time_;
        mutable struct tms last_cpu_time_;
        const unsigned int number_of_cpus_;
        std::chrono::time_point<std::chrono::high_resolution_clock> last_clock_time_;
};

#endif