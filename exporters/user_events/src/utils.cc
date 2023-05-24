
// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/user_events/logs/utils.h"

OPENTELEMETRY_BEGIN_NAMESPACE

namespace exporter {
namespace user_events {
namespace utils {

namespace api_common = opentelemetry::common;

const int kAttributeValueSize = 16;

void PopulateAttribute(nostd::string_view key,
                       const api_common::AttributeValue &value,
                       ehd::EventBuilder &event_builder) noexcept
{
  static_assert(nostd::variant_size<api_common::AttributeValue>::value == kAttributeValueSize,
                "AttributeValue has changed, update PopulateAttributeValue");

  if (nostd::holds_alternative<bool>(value))
  {
    event_builder.AddValue("u8", nostd::get<bool>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<int>(value))
  {
    event_builder.AddValue("s32", nostd::get<int>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<int64_t>(value))
  {
    event_builder.AddValue("s64", nostd::get<int64_t>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<unsigned int>(value))
  {
    event_builder.AddValue("u32", nostd::get<unsigned int>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<uint64_t>(value))
  {
    event_builder.AddValue("u64", nostd::get<uint64_t>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<double>(value), event_field_format_default)
  {
    event_builder.AddValue("f64", nostd::get<double>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<double>(value), event_field_format_default)
  {
    event_builder.AddValue("f64", nostd::get<double>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<const char *>(value))
  {

  }
  else if (nostd::holds_alternative<nostd::string_view>(value))
  {

  }
  else if (nostd::holds_alternative<nostd::span<const uint8_t>>(value))
  {

  }
  else if (nostd::holds_alternative<nostd::span<const int>>(value))
  {

  }
  else if (nostd::holds_alternative<nostd::span<const int64_t>>(value))
  {

  }
  else if (nostd::holds_alternative<nostd::span<const unsigned int>>(value))
  {

  }
  else if (nostd::holds_alternative<nostd::span<const uint64_t>>(value))
  {

  }
  else if (nostd::holds_alternative<nostd::span<const double>>(value))
  {

  }
  else if (nostd::holds_alternative<nostd::span<const bool>>(value))
  {

  }
  else if (nostd::holds_alternative<nostd::span<const nostd::string_view>>(value))
  {

  }
  else
  {
  }
  
}

}  // namespace utils
}  // namespace user_events
}  // namespace exporter

OPENTELEMETRY_END_NAMESPACE