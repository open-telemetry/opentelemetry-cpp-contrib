// Copyright 2023, OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "opentelemetry/version.h"

#ifndef OPENTELEMETRY_CONTRIB_PROMETHEUS_PUSH_API
#  if defined(__clang__)
#    define OPENTELEMETRY_CONTRIB_PROMETHEUS_PUSH_API __attribute__((visibility("default")))
#  elif defined(__GNUC__)
#    define OPENTELEMETRY_CONTRIB_PROMETHEUS_PUSH_API __attribute__((visibility("default")))
#  else
#    define OPENTELEMETRY_CONTRIB_PROMETHEUS_PUSH_API
#  endif
#endif

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace metrics
{

/**
 * Struct to hold Prometheus exporter options.
 */
struct OPENTELEMETRY_CONTRIB_PROMETHEUS_PUSH_API PrometheusPushExporterOptions
{
  std::string host;
  std::string port;
  std::string jobname;
  std::unordered_map<std::string, std::string> labels;
  std::string username;
  std::string password;

  std::size_t max_collection_size = 2000;

  // Populating target_info
  bool populate_target_info = true;

  // Populating otel_scope_name/otel_scope_labels attributes
  bool without_otel_scope = false;

  inline PrometheusPushExporterOptions() noexcept {}
};

}  // namespace metrics
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
