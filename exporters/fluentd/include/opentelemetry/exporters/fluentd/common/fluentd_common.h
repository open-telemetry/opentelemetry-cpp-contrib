
// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "opentelemetry/common/attribute_value.h"
#include "opentelemetry/exporters/fluentd/common/fluentd_logging.h"
#include "opentelemetry/nostd/string_view.h"
#include "opentelemetry/version.h"

#include "nlohmann/json.hpp"

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

} // namespace common
} // namespace fluentd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
