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

// @@@ (don't include) #include "AgentLogger.h"
#include <log4cxx/logger.h>
#include <log4cxx/rolling/rollingfileappenderskeleton.h>
#include <log4cxx/xml/domconfigurator.h>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include "AgentLogger.h"

using namespace log4cxx;
using namespace log4cxx::xml;
using namespace log4cxx::helpers;

/*
 * Initialize the log context names.
 * TODO: Modify the names while implementing core logic.
 */
const char* LogContext::AGENT = "agent";
const char* LogContext::TX_SERVICE = "agent.tx";
const char* LogContext::CONFIG = "agent.config";
const char* LogContext::ERROR = "agent.error";
const char* LogContext::REPORT = "agent.report";
const char* LogContext::SNAPSHOT = "agent.snapshot";
const char* LogContext::INSTRUMENT = "agent.instrument";
const char* LogContext::CORRELATION = "agent.correlation";
const char* LogContext::EXIT_HTTP = "agent.exit.http";
const char* LogContext::EXIT_CACHING = "agent.exit.caching";
const char* LogContext::CUSTOM_METRICS = "agent.custom_metrics";

log4cxx::LevelPtr DebugLoggingManager::m_previousLogLevel;

bool initLogging(const boost::filesystem::path& configFilePath)
{
    if (!configFilePath.is_absolute() || !boost::filesystem::exists(configFilePath))
    {
        logStartupError(boost::format("Log config file path invalid: %1%") % configFilePath.string());
        return false;
    }

    std::string pidStr(boost::lexical_cast<std::string>(getpid()));
    MDC::put("pid", pidStr);
    //MDC::put("version", std::string(AGENT_VERSION));

    DOMConfigurator::configure(configFilePath.native());


    return true;
}

AgentLogger getLogger(const std::string& name)
{
    LoggerPtr loggerPtr(Logger::getLogger(name));
    if (!loggerPtr)
    {
        logStartupError(boost::format("Cannot initialize log4cxx - null logger: %1%") % name);
        return Logger::getRootLogger();
    }
    return loggerPtr;
}
