/// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

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
    event_builder_.AddStruct("PartA", 1);
    utils::PopulateAttribute("__csver__", static_cast<uint16_t>(0x400), event_builder_);

    // event_builder_.AddStruct("PartB", 2);

    // event_builder_.AddStruct("PartC", 3);
}

void Recordable::SetSeverity(api_logs::Severity severity) noexcept
{
    uint8_t severity_value = static_cast<uint8_t>(severity);
    if (severity_value == 0 || severity_value > 24)
    {
        // TODO: log error
        severity_value = 1;
    }

    level_index_ = (severity_value - 1) >> 2;

    event_builder_.AddValue("severityNumber", static_cast<uint16_t>(severity_value), event_field_format_default);
}

void Recordable::SetBody(const opentelemetry::common::AttributeValue &message) noexcept
{
  utils::PopulateAttribute("body", message, event_builder_);
}

void Recordable::SetEventId(int64_t id, nostd::string_view name) noexcept
{
  utils::PopulateAttribute("eventId", id, event_builder_);
  if (!name.empty()) {
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

void Recordable::SetAttribute(
    nostd::string_view key,
    const opentelemetry::common::AttributeValue &value) noexcept
{
  // TODO: implement attributes
}

void Recordable::SetTimestamp(
    opentelemetry::common::SystemTimestamp timestamp) noexcept
{
  // TODO: convert to nanoseconds
}

} // namespace logs
} // namespace user_events
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
