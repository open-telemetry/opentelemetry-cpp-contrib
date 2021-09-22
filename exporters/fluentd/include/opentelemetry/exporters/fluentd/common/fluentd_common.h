
// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "opentelemetry/version.h"
#include "opentelemetry/nostd/string_view.h"
#include "opentelemetry/common/attribute_value.h"
#include "opentelemetry/exporters/fluentd/common/fluentd_logging.h"

#include "nlohmann/json.hpp"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace fluentd
{
namespace common
{
using FluentdLog = nlohmann::json;


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
 * Export mode - async and sync
 */

enum class ExportMode {
  SYNC_MODE,
  ASYNC_MODE
};

// Ref. https://github.com/fluent/fluentd/wiki/Forward-Protocol-Specification-v1
enum class TransportFormat
{
  kMessage,
  kForward,
  kPackedForward,
  kCompressedPackedForward
};

/**
 * Struct to hold fluentd  exporter options.
 */
struct FluentdExporterOptions
{
  // The endpoint to export to. By default the OpenTelemetry Collector's default endpoint.
  TransportFormat format = TransportFormat::kForward;
  std::string tag = "tag.service";
  std::string endpoint;
  ExportMode export_mode = ExportMode::ASYNC_MODE;
  size_t retry_count = 2;        // number of retries before drop
  size_t max_queue_size = 16384;  // max events buffer size
  size_t wait_interval_ms = 0;    // default wait interval between batches
  bool convert_event_to_trace = false ; // convert events to trace
};

constexpr int kAttributeValueSize = 15;

void inline PopulateAttribute(nlohmann::json &attribute,
                       nostd::string_view key,
                       const opentelemetry::common::AttributeValue &value)
{
  // Assert size of variant to ensure that this method gets updated if the variant
  // definition changes
  static_assert(
      nostd::variant_size<opentelemetry::common::AttributeValue>::value == kAttributeValueSize+1,
      "AttributeValue contains unknown type");

  if (nostd::holds_alternative<bool>(value))
  {
    attribute[key.data()] = nostd::get<bool>(value);
  }
  else if (nostd::holds_alternative<int>(value))
  {
    attribute[key.data()] = nostd::get<int>(value);
  }
  else if (nostd::holds_alternative<int64_t>(value))
  {
    attribute[key.data()] = nostd::get<int64_t>(value);
  }
  else if (nostd::holds_alternative<unsigned int>(value))
  {
    attribute[key.data()] = nostd::get<unsigned int>(value);
  }
  else if (nostd::holds_alternative<uint64_t>(value))
  {
    attribute[key.data()] = nostd::get<uint64_t>(value);
  }
  else if (nostd::holds_alternative<double>(value))
  {
    attribute[key.data()] = nostd::get<double>(value);
  }
   else if (nostd::holds_alternative<const char *>(value))
  {
    LOG_DEBUG( " LALIT -> Setting const char attribute env_properties");
    attribute[key.data()] =
        std::string(nostd::get<const char *>(value));

  }
  else if (nostd::holds_alternative<nostd::string_view>(value))
  {
    LOG_DEBUG( " LALIT -> Setting string attribute env_properties");
    attribute[key.data()] =
        std::string(nostd::get<nostd::string_view>(value).data());
        // nostd::string_view(nostd::get<nostd::string_view>(value).data(),
        // nostd::get<nostd::string_view>(value).size());
  }
  else if (nostd::holds_alternative<nostd::span<const uint8_t>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const uint8_t>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const bool>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const bool>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const int>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const int>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const int64_t>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const int64_t>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const unsigned int>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const unsigned int>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const uint64_t>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const uint64_t>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const double>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const double>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const nostd::string_view>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const nostd::string_view>>(value))
    {
      attribute[key.data()].push_back(std::string(val.data(), val.size()));
    }
  }
}


} // common
} // fluentd
} // exporter
OPENTELEMETRY_END_NAMESPACE