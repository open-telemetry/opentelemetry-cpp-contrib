/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <opentelemetry/logs/severity.h>

#include <glog/logging.h>

namespace google
{

// The LogMessageTime class was introduced sometime in v0.6.0
// Since there is no versioning available in this lib, we identify it based on the presence of this
// include guard that was added in the required version
#if defined(GLOG_EXPORT_H)
#  define GLOG_VERSION_HAS_LOGMESSAGETIME
#endif

class OpenTelemetrySink : public google::LogSink
{
public:
  static inline opentelemetry::logs::Severity levelToSeverity(int level) noexcept
  {
    using opentelemetry::logs::Severity;

    switch (level)
    {
      case google::GLOG_FATAL:
        return Severity::kFatal;
      case google::GLOG_ERROR:
        return Severity::kError;
      case google::GLOG_WARNING:
        return Severity::kWarn;
      case google::GLOG_INFO:
        return Severity::kInfo;
      default:
        return Severity::kInvalid;
    }
  }

#if defined(GLOG_VERSION_HAS_LOGMESSAGETIME)
  void send(google::LogSeverity,
            const char *,
            const char *,
            int,
            const google::LogMessageTime &,
            const char *,
            size_t) override;
#else
  void send(google::LogSeverity,
            const char *,
            const char *,
            int,
            const struct ::tm *,
            const char *,
            size_t) override;
#endif
};

}  // namespace google
