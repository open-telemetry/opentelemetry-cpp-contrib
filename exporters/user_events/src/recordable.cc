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

Recordable::Recordable() noexcept {}

void Recordable::SetSeverity(api_logs::Severity severity) noexcept
{
  uint8_t severity_value = static_cast<uint8_t>(severity);
  if (severity_value > 24)
  {
    OTEL_INTERNAL_LOG_ERROR(
        "[user_events Log Exporter] Recordable: invalid severity value: " << severity_value);
    severity_value = 0;
  }

  severity_    = severity_value;
  level_index_ = (severity_value - 1) >> 2;
}

void Recordable::SetBody(const opentelemetry::common::AttributeValue &message) noexcept
{
  if (severity_ == 0)
  {
    OTEL_INTERNAL_LOG_ERROR("[user_events Log Exporter] Recordable: severity is not set.");
    return;
  }

  auto event_name = !event_name_.empty() ? event_name_.data() : "OpenTelemetryLogs";

  event_builder_.Reset(event_name);
  utils::PopulateAttribute("__csver__", static_cast<uint16_t>(0x400), event_builder_);

  event_builder_.AddStruct("PartB", 1, 0, &cs_part_b_bookmark_);
  utils::PopulateAttribute("_typeName", "Log", event_builder_);

  event_builder_.AddValue("severityNumber", static_cast<uint16_t>(severity_),
                          event_field_format_default);
  auto severity_text = api_logs::SeverityNumToText[static_cast<uint32_t>(severity_)].data();
  event_builder_.AddString<char>("severityText", severity_text, event_field_format_default);
  cs_part_b_bookmark_size_ = 4;  // with the below body counted because it is available.

  if (has_event_id_)
  {
    utils::PopulateAttribute("eventId", event_id_, event_builder_);
    cs_part_b_bookmark_size_++;
  }

  utils::PopulateAttribute("body", message, event_builder_);
}

void Recordable::SetEventId(int64_t id, nostd::string_view name) noexcept
{
  has_event_id_ = true;
  event_id_     = id;
  event_name_   = name;
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
  if (cs_part_b_bookmark_size_ == 0)
  {
    // Part B is mandatory for exporting to user_events.
    OTEL_INTERNAL_LOG_ERROR("[user_events Log Exporter] Recordable: no data to export.");
    return false;
  }
  else
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
