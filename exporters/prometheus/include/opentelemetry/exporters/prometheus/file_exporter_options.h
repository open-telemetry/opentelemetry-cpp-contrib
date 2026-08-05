// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

#include "opentelemetry/version.h"

#ifndef OPENTELEMETRY_CONTRIB_PROMETHEUS_FILE_API
#  if defined(__clang__)
#    define OPENTELEMETRY_CONTRIB_PROMETHEUS_FILE_API __attribute__((visibility("default")))
#  elif defined(__GNUC__)
#    define OPENTELEMETRY_CONTRIB_PROMETHEUS_FILE_API __attribute__((visibility("default")))
#  else
#    define OPENTELEMETRY_CONTRIB_PROMETHEUS_FILE_API
#  endif
#endif

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace metrics
{

/**
 * Struct to hold Prometheus file exporter options.
 * @note Available placeholder for file_pattern and alias_pattern:
 *     %Y:  writes year as a 4 digit decimal number
 *     %y:  writes last 2 digits of year as a decimal number (range [00,99])
 *     %m:  writes month as a decimal number (range [01,12])
 *     %j:  writes day of the year as a decimal number (range [001,366])
 *     %d:  writes day of the month as a decimal number (range [01,31])
 *     %w:  writes weekday as a decimal number, where Sunday is 0 (range [0-6])
 *     %H:  writes hour as a decimal number, 24 hour clock (range [00-23])
 *     %I:  writes hour as a decimal number, 12 hour clock (range [01,12])
 *     %M:  writes minute as a decimal number (range [00,59])
 *     %S:  writes second as a decimal number (range [00,60])
 *     %F:  equivalent to "%Y-%m-%d" (the ISO 8601 date format)
 *     %T:  equivalent to "%H:%M:%S" (the ISO 8601 time format)
 *     %R:  equivalent to "%H:%M"
 *     %N:  rotate index, start from 0
 *     %n:  rotate index, start from 1
 */
struct OPENTELEMETRY_CONTRIB_PROMETHEUS_FILE_API PrometheusFileExporterOptions
{
  std::string file_pattern                 = "%Y-%m-%d.prometheus.%N.log";
  std::string alias_pattern                = "%Y-%m-%d.prometheus.log";
  std::chrono::microseconds flush_interval = std::chrono::microseconds{30000000};
  std::size_t flush_count                  = 256;
  std::size_t file_size                    = static_cast<std::size_t>(20) * 1024 * 1024;
  std::size_t rotate_size                  = 3;

  // Populating target_info
  bool populate_target_info = true;

  // Populating otel_scope_name/otel_scope_labels attributes
  bool without_otel_scope = false;

  inline PrometheusFileExporterOptions() noexcept {}
};

}  // namespace metrics
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
