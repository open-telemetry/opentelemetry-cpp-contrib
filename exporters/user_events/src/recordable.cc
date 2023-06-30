/// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/user_events/logs/recordable.h"
#include "opentelemetry/sdk/common/global_log_handler.h"

#include <chrono>
#include <map>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace user_events
{
namespace logs
{

namespace api_logs = opentelemetry::logs;

Recordable::Recordable() noexcept
{
  event_builder_.Reset("OpenTelemetry-Logs");

  utils::PopulateAttribute("__csver__", static_cast<uint16_t>(0x400), event_builder_);
}

void Recordable::SetSeverity(api_logs::Severity severity) noexcept
{
  uint8_t severity_value = static_cast<uint8_t>(severity);
  if (severity_value == 0 || severity_value > 24)
  {
    OTEL_INTERNAL_LOG_ERROR(
        "[user_events Log Exporter] Recordable: invalid severity value: " << severity_value);
    severity_value = 1;
  }

  level_index_ = (severity_value - 1) >> 2;

  cs_part_b_bookmark_size_ += 2;
  event_builder_.AddValue("severityNumber", static_cast<uint16_t>(severity_value),
                          event_field_format_default);
  auto severity_text = api_logs::SeverityNumToText[static_cast<uint32_t>(severity_value)].data();
  event_builder_.AddString<char>("severityText", severity_text, event_field_format_default);
}

void Recordable::SetBody(const opentelemetry::common::AttributeValue &message) noexcept
{
  // Set intial bookmark size to 1 for body below.
  cs_part_b_bookmark_size_++;
  event_builder_.AddStruct("PartB", 1, 0, &cs_part_b_bookmark_);
  utils::PopulateAttribute("body", message, event_builder_);
}

void Recordable::SetEventId(int64_t id, nostd::string_view name) noexcept
{
  cs_part_b_bookmark_size_++;
  utils::PopulateAttribute("eventId", id, event_builder_);
  if (!name.empty())
  {
    cs_part_b_bookmark_size_++;
    utils::PopulateAttribute("name", name, event_builder_);
  }
}

void Recordable::SetTraceId(const opentelemetry::trace::TraceId &trace_id) noexcept
{
  // optional for logs.
}

void Recordable::SetSpanId(const opentelemetry::trace::SpanId &span_id) noexcept
{
  // optional for logs.
}

//
// Attributes should be set consecutively without other fields, so we can use the bookmark to do
// backpatching.
//
void Recordable::SetAttribute(nostd::string_view key,
                              const opentelemetry::common::AttributeValue &value) noexcept
{
  if (cs_part_c_bookmark_size_ == 0)
  {
    event_builder_.AddStruct("PartC", 1, 0, &cs_part_c_bookmark_);
  }

  cs_part_c_bookmark_size_++;
  utils::PopulateAttribute(key, value, event_builder_);
}

void Recordable::SetTimestamp(opentelemetry::common::SystemTimestamp timestamp) noexcept
{
  // Timestamp is optional because timestamp is always recorded in the header of user_events.
}

bool Recordable::PrepareExport() noexcept
{
  if (cs_part_b_bookmark_size_ > 0)
  {
    event_builder_.SetStructFieldCount(cs_part_b_bookmark_, cs_part_b_bookmark_size_);
  }
  if (cs_part_c_bookmark_size_ > 0)
  {
    event_builder_.SetStructFieldCount(cs_part_c_bookmark_, cs_part_c_bookmark_size_);
  }

  return true;
}

}  // namespace logs
}  // namespace user_events
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
