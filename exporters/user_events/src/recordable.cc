/// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/sdk/common/global_log_handler.h"
#include "opentelemetry/exporters/user_events/logs/recordable.h"

#include <chrono>
#include <map>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace user_events {
namespace logs {

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
        // TODO: log error
        OTEL_INTERNAL_LOG_ERROR("[user_events Log Exporter] Recordable: invalid severity value: " << severity_value);
        severity_value = 1;
    }

    level_index_ = (severity_value - 1) >> 2;

    bookmark_size_ += 2;
    event_builder_.AddValue("severityNumber", static_cast<uint16_t>(severity_value), event_field_format_default);
    event_builder_.AddString<char>("severityText", api_logs::SeverityNumToText[static_cast<uint32_t>(severity_value)].data(), event_field_format_default);
}

void Recordable::SetBody(const opentelemetry::common::AttributeValue &message) noexcept
{
  if (bookmark_size_ > 0)
  {
      event_builder_.SetStructFieldCount(bookmark_, bookmark_size_);
  }

  // reset bookmark for EventHeaderBuilder patch.
  bookmark_ = 0;

  // Set intial bookmark size to 1 for body below.
  bookmark_size_ = 1;
  event_builder_.AddStruct("PartB", 1, 0, &bookmark_);
  utils::PopulateAttribute("body", message, event_builder_);
}

void Recordable::SetEventId(int64_t id, nostd::string_view name) noexcept
{
  bookmark_size_++;
  utils::PopulateAttribute("eventId", id, event_builder_);
  if (!name.empty()) {
    bookmark_size_++;
    utils::PopulateAttribute("name", name, event_builder_);
  }
}

void Recordable::SetTraceId(const opentelemetry::trace::TraceId &trace_id) noexcept
{
  // TODO: implement
}

void Recordable::SetSpanId(const opentelemetry::trace::SpanId &span_id) noexcept
{
  // TODO: implement
}

//
// Attributes should be set consecutively without other fields, so we can use the bookmark to do backpatching.
//
void Recordable::SetAttribute(
    nostd::string_view key,
    const opentelemetry::common::AttributeValue &value) noexcept
{
  if (bookmark_size_ == 0)
  {
    event_builder_.AddStruct("PartC", 1, 0, &bookmark_);
  }
  bookmark_size_++;

  utils::PopulateAttribute(key, value, event_builder_);
}

void Recordable::SetTimestamp(
    opentelemetry::common::SystemTimestamp timestamp) noexcept
{
  // TODO: convert to nanoseconds
}

bool Recordable::PrepareExport() noexcept
{
  if (bookmark_size_ > 0)
  {
    event_builder_.SetStructFieldCount(bookmark_, bookmark_size_);
  }

  return true;
}

} // namespace logs
} // namespace user_events
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
