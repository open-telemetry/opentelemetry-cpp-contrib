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


#ifndef __LOGGER_H_
#define __LOGGER_H_

#include <string>
#include <log4cxx/logger.h>
#include <log4cxx/logmanager.h>
#include <iostream>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>

typedef log4cxx::LoggerPtr AgentLogger;

bool initLogging(const boost::filesystem::path& configFilePath);
AgentLogger getLogger(const std::string& name);

/* this is cloned from log4cxx locationinfo.h */
#if defined(_MSC_VER)
    #if _MSC_VER >= 1300
        #define __LOG4CXX_FUNC__ __FUNCSIG__
    #endif
#else
    #if defined(__GNUC__)
        #define __LOG4CXX_FUNC__ __PRETTY_FUNCTION__
    #endif
#endif
#if !defined(__LOG4CXX_FUNC__)
    #define __LOG4CXX_FUNC__ ""
#endif
/* */

#ifdef _WIN32
#undef ERROR
#endif

/*
    *TODO: Change the following context names while writing the core logic.*
*/
struct LogContext
{
    static const char* AGENT;
    static const char* TX_SERVICE;
    static const char* CONFIG;
    static const char* ERROR;
    static const char* REPORT;
    static const char* SNAPSHOT;
    static const char* INSTRUMENT;
    static const char* CORRELATION;
    static const char* EXIT_HTTP;
    static const char* EXIT_CACHING;
    static const char* CUSTOM_METRICS;
};

/**
 * Helper class that is used to increase the logging level to DEBUG and then
 * reset it back to normal.
 * It can be used in future to forcefully adjust the logging level.
 */
class DebugLoggingManager
{
    public:
        inline static void enable()
        {
            log4cxx::spi::LoggerRepositoryPtr repository(log4cxx::LogManager::getLoggerRepository());
            m_previousLogLevel = repository->getThreshold();
            if (!m_previousLogLevel->equals(log4cxx::Level::getDebug()))
                repository->setThreshold(log4cxx::Level::getDebug());
        }

        inline static void reset()
        {
            if (m_previousLogLevel)
                log4cxx::LogManager::getLoggerRepository()->setThreshold(m_previousLogLevel);
        }

    private:
        DebugLoggingManager() {}
        static log4cxx::LevelPtr m_previousLogLevel;
};

template<class charT, class Traits>
inline void logStartupError(const boost::basic_format<charT,Traits>& f)
{
    std::cerr << f << std::endl;
}

inline void logStartupError(const char* message)
{
    std::cerr << message << std::endl;
}

#endif /* __LOGGER_H_ */
