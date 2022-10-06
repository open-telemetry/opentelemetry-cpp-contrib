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
    long TotalElapsedTime();

    // returns user and system time separately from beginning
    void TotalElapsedSystemAndUserTime(long &system_time, long &user_time);

    // returns cpu time (user + system) since last call to Last*Time()
    double LastElapsedTime();

   // returns user and system time separately since last call to Last*Time()
    void LastElapsedSystemAndUserTime(long &system_time, long &user_time);

    private:
        static long clock_ticks_per_sec_;
        struct tms start_time_;
        mutable struct tms last_time_;
};

#endif