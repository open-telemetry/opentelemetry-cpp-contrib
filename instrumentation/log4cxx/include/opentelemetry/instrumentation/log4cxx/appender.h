/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <opentelemetry/logs/severity.h>

#include <log4cxx/appenderskeleton.h>
#include <log4cxx/helpers/stringhelper.h>
#include <log4cxx/level.h>

namespace log4cxx
{

class OpenTelemetryAppender : public AppenderSkeleton
{
public:
  DECLARE_LOG4CXX_OBJECT(OpenTelemetryAppender)
  BEGIN_LOG4CXX_CAST_MAP()
  LOG4CXX_CAST_ENTRY(OpenTelemetryAppender)
  LOG4CXX_CAST_ENTRY_CHAIN(AppenderSkeleton)
  END_LOG4CXX_CAST_MAP()

  static const std::string& libraryVersion()
  {
    static const std::string kLibraryVersion = std::to_string(LOG4CXX_VERSION_MAJOR) + "." +
                                               std::to_string(LOG4CXX_VERSION_MINOR) + "." +
                                               std::to_string(LOG4CXX_VERSION_PATCH);
    return kLibraryVersion;
  }

  static inline opentelemetry::logs::Severity levelToSeverty(int level) noexcept
  {
    using log4cxx::Level;
    using opentelemetry::logs::Severity;

    switch (level)
    {
      case Level::FATAL_INT:
        return Severity::kFatal;
      case Level::ERROR_INT:
        return Severity::kError;
      case Level::WARN_INT:
        return Severity::kWarn;
      case Level::INFO_INT:
        return Severity::kInfo;
      case Level::DEBUG_INT:
        return Severity::kDebug;
      case Level::TRACE_INT:
      case Level::ALL_INT:
        return Severity::kTrace;
      case Level::OFF_INT:
      default:
        return Severity::kInvalid;
    }
  }

  OpenTelemetryAppender() = default;

  void close() override {}

  inline bool requiresLayout() const override { return false; }

  void append(const spi::LoggingEventPtr &event, helpers::Pool &) override;
};

IMPLEMENT_LOG4CXX_OBJECT(OpenTelemetryAppender)

}  // namespace log4cxx
