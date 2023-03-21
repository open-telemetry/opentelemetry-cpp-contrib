/*
* Copyright 2022, OpenTelemetry Authors. 
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef APACHETRACING_H
#define APACHETRACING_H

#include <string>
#include <vector>
#include <httpd.h>
#include <http_log.h>

/**
 * Class that is used to log agent logs into Apache error logs,
 * based on the flag "m_traceAsErrorFromUser" set by user.
 */
class ApacheTracing
{
public:

    static bool m_traceAsErrorFromUser; // trace config from user

    enum ApacheTraceStates
    {
        UNINITIALIZED,
        NOTRACE,
        TRACE_AS_ERROR
    };

    static void writeTrace(server_rec* s, const char* funcName, const char* note, ...);
    static void writeError(server_rec* s, const char* funcName, const char* note, ...);
    static void logStartupTrace(server_rec *s);

private:
    static ApacheTraceStates m_state;
    static std::vector<std::string> m_startupTrace; // holds log message until traceLevel is set
};
#endif
