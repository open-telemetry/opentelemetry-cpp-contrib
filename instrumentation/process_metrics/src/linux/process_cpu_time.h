// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef __linux__
#include <time.h>
#include <sys/times.h>

class ProcessCpuTime
{
    public:
    ProcessCpuTime();

    // returns cpu time (user + system) from beginning
    double TotalElapsedTime();

    // returns user and system time separately from beginning
    void TotalElapsedSystemAndUserTime(double &system_time, double &user_time);

    // returns cpu time (user + system) since last call to Last*Time()
    double LastElapsedTime();

   // returns user and system time separately since last call to Last*Time()
    void LastElapsedSystemAndUserTime(double &system_time, double &user_time);


    private:
        static long clock_ticks_per_sec_;
        struct tms start_time_;
        mutable struct tms last_time_;
};

#endif