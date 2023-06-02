
// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/user_events/logs/utils.h"

OPENTELEMETRY_BEGIN_NAMESPACE

namespace exporter
{
namespace user_events
{
namespace utils
{

namespace api_common = opentelemetry::common;

const int kAttributeValueSize = 16;

void PopulateAttribute(nostd::string_view key,
                       const api_common::AttributeValue &value,
                       ehd::EventBuilder &event_builder) noexcept
{
  static_assert(nostd::variant_size<api_common::AttributeValue>::value == kAttributeValueSize,
                "AttributeValue has changed, update PopulateAttributeValue");

  const char *key_name = key.data();

  if (nostd::holds_alternative<bool>(value))
  {
    event_builder.AddValue(key_name, nostd::get<bool>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<int>(value))
  {
    event_builder.AddValue(key_name, nostd::get<int>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<int64_t>(value))
  {
    event_builder.AddValue(key_name, nostd::get<int64_t>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<unsigned int>(value))
  {
    event_builder.AddValue(key_name, nostd::get<unsigned int>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<uint64_t>(value))
  {
    event_builder.AddValue(key_name, nostd::get<uint64_t>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<double>(value), event_field_format_default)
  {
    event_builder.AddValue(key_name, nostd::get<double>(value), event_field_format_default);
  }
  else if (nostd::holds_alternative<const char *>(value))
  {
    event_builder.AddString<char>(key_name, nostd::get<const char *>(value),
                                  event_field_format_default);
  }
  else if (nostd::holds_alternative<nostd::string_view>(value))
  {
    event_builder.AddString<char>(key_name, nostd::get<nostd::string_view>(value).data(),
                                  event_field_format_default);
  }
  else if (nostd::holds_alternative<nostd::span<const uint8_t>>(value))
  {
    // TODO: implement
  }
  else if (nostd::holds_alternative<nostd::span<const int>>(value))
  {}
  else if (nostd::holds_alternative<nostd::span<const int64_t>>(value))
  {}
  else if (nostd::holds_alternative<nostd::span<const unsigned int>>(value))
  {}
  else if (nostd::holds_alternative<nostd::span<const uint64_t>>(value))
  {}
  else if (nostd::holds_alternative<nostd::span<const double>>(value))
  {}
  else if (nostd::holds_alternative<nostd::span<const bool>>(value))
  {}
  else if (nostd::holds_alternative<nostd::span<const nostd::string_view>>(value))
  {}
}

}  // namespace utils
}  // namespace user_events
}  // namespace exporter

OPENTELEMETRY_END_NAMESPACE