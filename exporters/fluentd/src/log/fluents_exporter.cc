// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#define HAVE_CONSOLE_LOG

#include "opentelemetry/exporters/fluentd/log/fluent_exporter.h"
#include "opentelemetry/exporters/fluentd/log/recordable.h"
#include "opentelemetry/ext/http/common/url_parser.h"

#include "opentelemetry/exporters/fluentd/common/fluentd_logging.h"

#include "nlohmann/json.hpp"

#include <cassert>

#include <iostream>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace logs
{
namespace fluentd
{

/**
 * @brief Scheme for tcp:// stream
*/
constexpr const char* kTCP = "tcp";

/**
 * @brief Scheme for udp:// datagram
*/
constexpr const char* kUDP = "udp";

/**
 * @brief Scheme for unix:// domain socket
*/
constexpr const char* kUNIX = "unix";

/**
 * @brief Create FluentD exporter with options
 * @param options 
*/
FluentdExporter::FluentdExporter(const FluentdExporterOptions &options) : options_(options)
{
  Initialize();
}

/**
 * @brief Create FluentD exporter with default options
*/
FluentdExporter::FluentdExporter() : options_(FluentdExporterOptions())
{
  Initialize();
}

/**
 * @brief Create new Recordable
 * @return Recordable
*/
std::unique_ptr<logs_sdk::Recordable> FluentdExporter::MakeRecordable() noexcept
{
  LOG_TRACE("LALIT: Make recordable");
  return std::unique_ptr<sdk::logs::Recordable>(new Recordable);
}

/**
 * @brief Export spans.
 * @param spans 
 * @return Export result.
*/
sdk::common::ExportResult FluentdExporter::Export(
    const nostd::span<std::unique_ptr<logs_sdk::Recordable>> &logs) noexcept
{
  LOG_ERROR("\nLALIT:Exporting...");

  // Return failure if this exporter has been shutdown
  if (is_shutdown_)
  {
    return sdk::common::ExportResult::kFailure;
  }

  // If no spans in Recordable, then return error.
  if (logs.size() == 0)
  {
    return sdk::common::ExportResult::kFailure;
  }

  json events = {};
  {
    // Write all Spans first
    json obj = json::array();
    obj.push_back(FLUENT_VALUE_LOG);
    json spanevents = json::array();
    for (auto &recordable : logs)
    {
      

