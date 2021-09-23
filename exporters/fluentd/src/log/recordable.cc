/// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/fluentd/log/recordable.h"
#include "opentelemetry/exporters/fluentd/common/fluentd_common.h"
#include "opentelemetry/exporters/fluentd/common/fluentd_logging.h"

#include <chrono>
#include <map>

using namespace nlohmann;

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace fluentd {
namespace logs {

void Recordable::SetSeverity(opentelemetry::logs::Severity severity) noexcept {
  json_["severityText"] =
      opentelemetry::logs::SeverityNumToText[static_cast<int>(severity)];
  json_["severityNumber"] = severity;
}

void Recordable::SetName(nostd::string_view name) noexcept {
  json_["name"] = name.data();
}

void Recordable::SetBody(nostd::string_view message) noexcept {
  json_["body"] = message.data();
}

void Recordable::SetTraceId(opentelemetry::trace::TraceId trace_id) noexcept {
  char trace_id_lower_base16[opentelemetry::trace::TraceId::kSize * 2] = {0};
  trace_id.ToLowerBase16(trace_id_lower_base16);
  json_[FLUENT_FIELD_TRACE_ID] = std::string(trace_id_lower_base16, 32);
}

void Recordable::SetSpanId(opentelemetry::trace::SpanId span_id) noexcept {
  char span_id_lower_base16[opentelemetry::trace::SpanId::kSize * 2] = {0};
  span_id.ToLowerBase16(span_id_lower_base16);
  json_[FLUENT_FIELD_SPAN_ID] = std::string(span_id_lower_base16, 16);
}

void Recordable::SetAttribute(
    nostd::string_view key,
    const opentelemetry::common::AttributeValue &value) noexcept {
  LOG_DEBUG(" LALIT:Setting env_properties");
  if (!json_.contains(FLUENT_FIELD_PROPERTIES)) {
    json_[FLUENT_FIELD_PROPERTIES] = nlohmann::json::object();
  }

  opentelemetry::exporter::fluentd::common::PopulateAttribute(
      json_[FLUENT_FIELD_PROPERTIES], key, value);
}

void Recordable::SetTimestamp(
    opentelemetry::common::SystemTimestamp timestamp) noexcept {
  json_["Timestamp"] = get_msgpack_eventtimeext(
      static_cast<int32_t>(std::chrono::duration_cast<std::chrono::seconds>(
                               timestamp.time_since_epoch())
                               .count()),
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          timestamp.time_since_epoch())
              .count() %
          1000000000);
}

} // namespace logs
} // namespace fluentd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE