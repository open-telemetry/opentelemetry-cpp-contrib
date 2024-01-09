/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <opentelemetry/common/attribute_value.h>
#include <opentelemetry/logs/log_record.h>
#include <opentelemetry/logs/severity.h>

#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/trivial.hpp>
#include <boost/version.hpp>

#include <chrono>
#include <functional>

namespace opentelemetry
{
namespace instrumentation
{
namespace boost_log
{

using IntMapper    = std::function<bool(const boost::log::record_view &, int &)>;
using StringMapper = std::function<bool(const boost::log::record_view &, std::string &)>;
using SeverityMapper =
    std::function<opentelemetry::logs::Severity(const boost::log::record_view &)>;
using TimeStampMapper =
    std::function<bool(const boost::log::record_view &, std::chrono::system_clock::time_point &)>;

using IntSetter       = std::function<void(opentelemetry::logs::LogRecord *, int)>;
using StringSetter    = std::function<void(opentelemetry::logs::LogRecord *, const std::string &)>;
using TimeStampSetter = std::function<void(opentelemetry::logs::LogRecord *,
                                           const std::chrono::system_clock::time_point &)>;

struct ValueMappers
{
  SeverityMapper ToSeverity;
  TimeStampMapper ToTimeStamp;
  StringMapper ToThreadId;
  StringMapper ToCodeFile;
  StringMapper ToCodeFunc;
  IntMapper ToCodeLine;
};

class OpenTelemetrySinkBackend
    : public boost::log::sinks::basic_sink_backend<boost::log::sinks::synchronized_feeding>
{
public:
  static const std::string &libraryVersion()
  {
    static const std::string kLibraryVersion = std::to_string(BOOST_VERSION / 100000) + "." +
                                               std::to_string(BOOST_VERSION / 100 % 1000) + "." +
                                               std::to_string(BOOST_VERSION % 100);
    return kLibraryVersion;
  }

  static inline opentelemetry::logs::Severity levelToSeverity(int level) noexcept
  {
    using namespace boost::log::trivial;
    using opentelemetry::logs::Severity;

    switch (level)
    {
      case severity_level::fatal:
        return Severity::kFatal;
      case severity_level::error:
        return Severity::kError;
      case severity_level::warning:
        return Severity::kWarn;
      case severity_level::info:
        return Severity::kInfo;
      case severity_level::debug:
        return Severity::kDebug;
      case severity_level::trace:
        return Severity::kTrace;
      default:
        return Severity::kInvalid;
    }
  }

  OpenTelemetrySinkBackend() noexcept : OpenTelemetrySinkBackend(ValueMappers{}) {}

  explicit OpenTelemetrySinkBackend(const ValueMappers &) noexcept;

  void consume(const boost::log::record_view &);

private:
  ValueMappers mappers_;
  std::array<TimeStampSetter, 2> set_timestamp_if_valid_;
  std::array<StringSetter, 2> set_thread_id_if_valid_;
  std::array<StringSetter, 2> set_file_path_if_valid_;
  std::array<StringSetter, 2> set_func_name_if_valid_;
  std::array<IntSetter, 2> set_code_line_if_valid_;
};

}  // namespace boost_log
}  // namespace instrumentation
}  // namespace opentelemetry
