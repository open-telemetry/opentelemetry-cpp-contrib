
// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "opentelemetry/common/attribute_value.h"
#include "opentelemetry/sdk/common/attribute_utils.h"
#include "opentelemetry/exporters/fluentd/common/fluentd_logging.h"
#include "opentelemetry/nostd/string_view.h"
#include "opentelemetry/version.h"

#include "nlohmann/json.hpp"

#include <chrono>

#ifndef ENABLE_LOGS_PREVIEW
#define ENABLE_LOGS_PREVIEW 1
#endif

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace fluentd {
namespace common {
using FluentdLog = nlohmann::json;

constexpr int kAttributeValueSize = 15;

void inline PopulateAttribute(
    nlohmann::json &attribute, nostd::string_view key,
    const opentelemetry::common::AttributeValue &value) {
  // Assert size of variant to ensure that this method gets updated if the
  // variant definition changes
  static_assert(
      nostd::variant_size<opentelemetry::common::AttributeValue>::value ==
          kAttributeValueSize + 1,
      "AttributeValue contains unknown type");

  if (nostd::holds_alternative<bool>(value)) {
    attribute[key.data()] = nostd::get<bool>(value);
  } else if (nostd::holds_alternative<int>(value)) {
    attribute[key.data()] = nostd::get<int>(value);
  } else if (nostd::holds_alternative<int64_t>(value)) {
    attribute[key.data()] = nostd::get<int64_t>(value);
  } else if (nostd::holds_alternative<unsigned int>(value)) {
    attribute[key.data()] = nostd::get<unsigned int>(value);
  } else if (nostd::holds_alternative<uint64_t>(value)) {
    attribute[key.data()] = nostd::get<uint64_t>(value);
  } else if (nostd::holds_alternative<double>(value)) {
    attribute[key.data()] = nostd::get<double>(value);
  } else if (nostd::holds_alternative<const char *>(value)) {
    attribute[key.data()] = std::string(nostd::get<const char *>(value));

  } else if (nostd::holds_alternative<nostd::string_view>(value)) {
    attribute[key.data()] =
        std::string(nostd::get<nostd::string_view>(value).data());
    // nostd::string_view(nostd::get<nostd::string_view>(value).data(),
    // nostd::get<nostd::string_view>(value).size());
  } else if (nostd::holds_alternative<nostd::span<const uint8_t>>(value)) {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const uint8_t>>(value)) {
      attribute[key.data()].push_back(val);
    }
  } else if (nostd::holds_alternative<nostd::span<const bool>>(value)) {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const bool>>(value)) {
      attribute[key.data()].push_back(val);
    }
  } else if (nostd::holds_alternative<nostd::span<const int>>(value)) {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const int>>(value)) {
      attribute[key.data()].push_back(val);
    }
  } else if (nostd::holds_alternative<nostd::span<const int64_t>>(value)) {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const int64_t>>(value)) {
      attribute[key.data()].push_back(val);
    }
  } else if (nostd::holds_alternative<nostd::span<const unsigned int>>(value)) {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const unsigned int>>(value)) {
      attribute[key.data()].push_back(val);
    }
  } else if (nostd::holds_alternative<nostd::span<const uint64_t>>(value)) {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const uint64_t>>(value)) {
      attribute[key.data()].push_back(val);
    }
  } else if (nostd::holds_alternative<nostd::span<const double>>(value)) {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const double>>(value)) {
      attribute[key.data()].push_back(val);
    }
  } else if (nostd::holds_alternative<nostd::span<const nostd::string_view>>(
                 value)) {
    attribute[key.data()] = {};
    for (const auto &val :
         nostd::get<nostd::span<const nostd::string_view>>(value)) {
      attribute[key.data()].push_back(std::string(val.data(), val.size()));
    }
  }
}

void inline PopulateOwnedAttribute(
    nlohmann::json &attribute, nostd::string_view key,
    const opentelemetry::sdk::common::OwnedAttributeValue &value) {
  // Assert size of variant to ensure that this method gets updated if the
  // variant definition changes
  static_assert(
      nostd::variant_size<opentelemetry::common::AttributeValue>::value ==
          kAttributeValueSize + 1,
      "AttributeValue contains unknown type");

  namespace common = opentelemetry::sdk::common;
  switch(value.index()) {
    case common::kTypeBool:
      attribute[key.data()] = nostd::get<bool>(value);
      break;
    case common::kTypeInt:
      attribute[key.data()] = nostd::get<int>(value);
      break;
    case common::kTypeUInt:
      attribute[key.data()] = nostd::get<unsigned int>(value);
      break;
    case common::kTypeInt64:
      attribute[key.data()] = nostd::get<int64_t>(value);
      break;
    case common::kTypeDouble:
      attribute[key.data()] = nostd::get<double>(value);
      break;
    case common::kTypeString:
      attribute[key.data()] = nostd::get<std::string>(value);
      break;
    case common::kTypeSpanBool:
      attribute[key.data()] = {};
      for (const auto &val : nostd::get<std::vector<bool>>(value)) {
        attribute[key.data()].push_back(val);
      }
      break;
    case common::kTypeSpanInt:
      attribute[key.data()] = {};
      for (const auto &val : nostd::get<std::vector<int>>(value)) {
        attribute[key.data()].push_back(val);
      }
      break;
    case common::kTypeSpanUInt:
      attribute[key.data()] = {};
      for (const auto &val : nostd::get<std::vector<unsigned int>>(value)) {
        attribute[key.data()].push_back(val);
      }
      break;
    case common::kTypeSpanInt64:
      attribute[key.data()] = {};
      for (const auto &val : nostd::get<std::vector<int64_t>>(value)) {
        attribute[key.data()].push_back(val);
      }
      break;
    case common::kTypeSpanDouble:
      attribute[key.data()] = {};
      for (const auto &val : nostd::get<std::vector<double>>(value)) {
        attribute[key.data()].push_back(val);
      }
      break;
    case common::kTypeSpanString:
      attribute[key.data()] = {};
      for (const auto &val :
          nostd::get<std::vector<std::string>>(value)) {
        attribute[key.data()].push_back(val);
      }
      break;
    case common::kTypeUInt64:
      attribute[key.data()] = nostd::get<uint64_t>(value);
      break;
    case common::kTypeSpanUInt64:
      attribute[key.data()] = {};
      for (const auto &val : nostd::get<std::vector<uint64_t>>(value)) {
        attribute[key.data()].push_back(val);
      }
      break;
    case common::kTypeSpanByte:
      attribute[key.data()] = {};
      for (const auto &val : nostd::get<std::vector<uint8_t>>(value)) {
        attribute[key.data()].push_back(val);
      }
      break;
    default:
      break;
  }
}

inline std::string AttributeValueToString(
    const opentelemetry::common::AttributeValue &value) {
  std::string result;
  if (nostd::holds_alternative<bool>(value)) {
    result = nostd::get<bool>(value) ? "true" : "false";
  } else if (nostd::holds_alternative<int>(value)) {
    result = std::to_string(nostd::get<int>(value));
  } else if (nostd::holds_alternative<int64_t>(value)) {
    result = std::to_string(nostd::get<int64_t>(value));
  } else if (nostd::holds_alternative<unsigned int>(value)) {
    result = std::to_string(nostd::get<unsigned int>(value));
  } else if (nostd::holds_alternative<uint64_t>(value)) {
    result = std::to_string(nostd::get<uint64_t>(value));
  } else if (nostd::holds_alternative<double>(value)) {
    result = std::to_string(nostd::get<double>(value));
  } else if (nostd::holds_alternative<const char*>(value)) {
    result = std::string(nostd::get<const char*>(value));
  } else if (nostd::holds_alternative<opentelemetry::nostd::string_view>(value)) {
    result = std::string(nostd::get<opentelemetry::nostd::string_view>(value).data());
  } else {
    LOG_WARN("[Fluentd Exporter] AttributeValueToString - "
             " Nested attributes not supported - ignored");
  }
  return result;
}

// Ref. https://github.com/fluent/fluentd/wiki/Forward-Protocol-Specification-v1
enum class TransportFormat {
  kMessage,
  kForward,
  kPackedForward,
  kCompressedPackedForward
};

/**
 * Struct to hold fluentd  exporter options.
 */
struct FluentdExporterOptions {
  // The endpoint to export to. By default the OpenTelemetry Collector's default
  // endpoint.
  TransportFormat format = TransportFormat::kForward;
  std::string tag = "tag.service";
  size_t retry_count = 2; // number of retries before drop
  uint32_t retry_delay_ms = 1000;
  std::string endpoint;
  bool convert_event_to_trace =
      false; // convert events to trace. Not used for Logs.
  bool include_trace_state_for_span = false;
};

static inline nlohmann::byte_container_with_subtype<std::vector<std::uint8_t>>
get_msgpack_eventtimeext(int32_t seconds = 0, int32_t nanoseconds = 0) {
  if ((seconds == 0) && (nanoseconds == 0)) {
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    auto duration = tp.time_since_epoch();
    seconds = static_cast<int32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(duration).count());
    nanoseconds = static_cast<int32_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() %
        1000000000);
  }
  nlohmann::byte_container_with_subtype<std::vector<std::uint8_t>> ts{
      std::vector<uint8_t>{0, 0, 0, 0, 0, 0, 0, 0}};
  for (int i = 3; i >= 0; i--) {
    ts[i] = seconds & 0xff;
    ts[i + 4] = nanoseconds & 0xff;
    seconds >>= 8;
    nanoseconds >>= 8;
  }
  ts.set_subtype(0x00);
  return ts;
}

} // namespace common
} // namespace fluentd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
