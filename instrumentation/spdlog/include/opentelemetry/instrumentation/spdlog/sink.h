/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <opentelemetry/logs/severity.h>

#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/version.h>

#include <mutex>

namespace spdlog
{
namespace sinks
{

template <typename Mutex>
class OpenTelemetrySink : public spdlog::sinks::base_sink<Mutex>
{
public:
  static const std::string &libraryVersion()
  {
    static const std::string kLibraryVersion = std::to_string(SPDLOG_VER_MAJOR) + "." +
                                               std::to_string(SPDLOG_VER_MINOR) + "." +
                                               std::to_string(SPDLOG_VER_PATCH);
    return kLibraryVersion;
  }

  static inline opentelemetry::logs::Severity levelToSeverity(int level) noexcept
  {
    namespace Level = spdlog::level;
    using opentelemetry::logs::Severity;

    switch (level)
    {
      case Level::critical:
        return Severity::kFatal;
      case Level::err:
        return Severity::kError;
      case Level::warn:
        return Severity::kWarn;
      case Level::info:
        return Severity::kInfo;
      case Level::debug:
        return Severity::kDebug;
      case Level::trace:
        return Severity::kTrace;
      case Level::off:
      default:
        return Severity::kInvalid;
    }
  }

protected:
  void sink_it_(const spdlog::details::log_msg &msg) override;
  void flush_() override {}
};

using opentelemetry_sink_mt = OpenTelemetrySink<std::mutex>;
using opentelemetry_sink_st = OpenTelemetrySink<spdlog::details::null_mutex>;

}  // namespace sinks
}  // namespace spdlog
