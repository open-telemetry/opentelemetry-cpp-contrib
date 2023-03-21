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

// file header
// apache headers
#include <stdarg.h>
#include <apr_strings.h>
#include <httpd.h>

#ifdef strtoul
#undef strtoul
#endif

#include <http_log.h> // ap_log_error and others

#include "ApacheTracing.h"

#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(otel_apache);
#endif

ApacheTracing::ApacheTraceStates ApacheTracing::m_state = ApacheTracing::ApacheTraceStates::UNINITIALIZED;
bool ApacheTracing::m_traceAsErrorFromUser = false;
std::vector<std::string> ApacheTracing::m_startupTrace;

void ApacheTracing::writeTrace(server_rec* s, const char* funcName, const char* format, ...)
{
    char note[8192];
    va_list args;

    va_start(args, format);
    vsnprintf(note, sizeof(note), format, args);
    va_end(args);

    if (m_state == TRACE_AS_ERROR && s)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, "mod_opentelemetry_webserver_sdk: %s: %s", funcName, note);
    }
    else if (m_state == UNINITIALIZED)
    {
        std::string trace(funcName);
        trace += ": ";
        trace += note;
        m_startupTrace.push_back(trace);
    }
}

void ApacheTracing::writeError(server_rec* s, const char* funcName, const char* format, ...)
{
    char note[8192];
    va_list args;

    va_start(args, format);
    vsnprintf(note, sizeof(note), format, args);
    va_end(args);

    if (s && funcName)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, "mod_opentelemetry_webserver_sdk: %s: %s", funcName, note);
    }
}

// Set the trace level and log the buffered startup trace into apache log
void ApacheTracing::logStartupTrace(server_rec* s)
{
    if (m_traceAsErrorFromUser)
    {
        m_state = TRACE_AS_ERROR;

        for (auto it = m_startupTrace.begin(); it != m_startupTrace.end(); it++)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, it->c_str());
        }
    }
    else // default to DEBUG
    {
        m_state = NOTRACE;
    }

    m_startupTrace.clear();
}
