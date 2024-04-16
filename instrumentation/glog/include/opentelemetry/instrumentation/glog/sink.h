/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <opentelemetry/logs/severity.h>

#include <glog/logging.h>

namespace google
{

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

  void send(google::LogSeverity,
            const char *,
            const char *,
            int,
            const google::LogMessageTime &,
            const char *,
            size_t) override;
};

}  // namespace google
