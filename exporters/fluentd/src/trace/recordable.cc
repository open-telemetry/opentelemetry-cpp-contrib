/// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/fluentd/trace/recordable.h"
#include "opentelemetry/exporters/fluentd/common/fluentd_common.h"
#include "opentelemetry/exporters/fluentd/common/fluentd_logging.h"

#include "opentelemetry/sdk/resource/resource.h"

#include <map>
#include <string>

using namespace nlohmann;
namespace fluentd_common = opentelemetry::exporter::fluentd::common;

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace fluentd {
namespace trace {
template <typename T> static inline json create_message(T ts, json body) {
  auto arr = json::array();
  arr.push_back(ts);
  arr.push_back(body);
  return arr;
}

// constexpr needs keys to be constexpr, const is next best to use.
static const std::map<opentelemetry::trace::SpanKind, std::string>
    kSpanKindMap = {
        {opentelemetry::trace::SpanKind::kClient, "CLIENT"},
        {opentelemetry::trace::SpanKind::kServer, "SERVER"},
        {opentelemetry::trace::SpanKind::kConsumer, "CONSUMER"},
        {opentelemetry::trace::SpanKind::kProducer, "PRODUCER"},
};

//
// See `attribute_value.h` for details.
//
const int kAttributeValueSize = 15;

void Recordable::SetIdentity(
    const opentelemetry::trace::SpanContext &span_context,
    opentelemetry::trace::SpanId parent_span_id) noexcept {
  char trace_id_lower_base16[opentelemetry::trace::TraceId::kSize * 2] = {0};
  span_context.trace_id().ToLowerBase16(trace_id_lower_base16);
  char span_id_lower_base16[opentelemetry::trace::SpanId::kSize * 2] = {0};
  span_context.span_id().ToLowerBase16(span_id_lower_base16);
  char parent_span_id_lower_base16[opentelemetry::trace::SpanId::kSize * 2] = {
      0};
  parent_span_id.ToLowerBase16(parent_span_id_lower_base16);
  options_[FLUENT_FIELD_SPAN_ID] = std::string(span_id_lower_base16, 16);
  options_[FLUENT_FIELD_SPAN_PARENTID] =
      std::string(parent_span_id_lower_base16, 16);
  options_[FLUENT_FIELD_TRACE_ID] = std::string(trace_id_lower_base16, 32);
}

void Recordable::SetAttribute(
    nostd::string_view key,
    const opentelemetry::common::AttributeValue &value) noexcept {
  if (!options_.contains(FLUENT_FIELD_PROPERTIES)) {
    options_[FLUENT_FIELD_PROPERTIES] = nlohmann::json::object();
  }

  fluentd_common::PopulateAttribute(options_[FLUENT_FIELD_PROPERTIES], key,
                                    value);
}

void Recordable::AddEvent(
    nostd::string_view name, opentelemetry::common::SystemTimestamp timestamp,
    const opentelemetry::common::KeyValueIterable &attributes) noexcept {
  nlohmann::json attrs = nlohmann::json::object(); // empty object
  attributes.ForEachKeyValue(
      [&](nostd::string_view key,
          opentelemetry::common::AttributeValue value) noexcept {
        fluentd_common::PopulateAttribute(attrs, key, value);
        return true;
      });

  // Event name
  attrs[FLUENT_FIELD_NAME] = name;
  auto ts = fluentd_common::get_msgpack_eventtimeext();
  events_.push_back(create_message(ts, attrs));
}

void Recordable::AddLink(
    const opentelemetry::trace::SpanContext &span_context,
    const opentelemetry::common::KeyValueIterable &attributes) noexcept {
  // TODO: Currently not supported by specs:
  // https://github.com/open-telemetry/opentelemetry-specification/blob/master/specification/trace/sdk_exporters/fluentd.md
}

void Recordable::SetStatus(opentelemetry::trace::StatusCode code,
                           nostd::string_view description) noexcept {
  if (code != opentelemetry::trace::StatusCode::kUnset) {
    options_["tags"]["otel.status_code"] = code;
    if (code == opentelemetry::trace::StatusCode::kError) {
      options_["tags"]["error"] = description;
    }
  }
}

void Recordable::SetName(nostd::string_view name) noexcept {
  // Span name.. Should this be tag name?
  options_[FLUENT_FIELD_NAME] = name.data();
}

void Recordable::SetResource(
    const opentelemetry::sdk::resource::Resource &resource) noexcept {
  // only tag attribute is supported by specs as of now.
  auto attributes = resource.GetAttributes();
  if (attributes.find("tag") != attributes.end()) {
    tag_ = nostd::get<std::string>(attributes["tag"]);
  }
}

void Recordable::SetStartTime(
    opentelemetry::common::SystemTimestamp start_time) noexcept {
  options_[FLUENT_FIELD_STARTTIME] = fluentd_common::get_msgpack_eventtimeext(
      static_cast<int32_t>(std::chrono::duration_cast<std::chrono::seconds>(
                               start_time.time_since_epoch())
                               .count()),
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          start_time.time_since_epoch())
              .count() %
          1000000000);
}

void Recordable::SetDuration(std::chrono::nanoseconds duration) noexcept {
  options_[FLUENT_FIELD_ENDTTIME] = fluentd_common::get_msgpack_eventtimeext();
  options_[FLUENT_FIELD_DURATION] = duration.count();
}

void Recordable::SetSpanKind(
    opentelemetry::trace::SpanKind span_kind) noexcept {
  auto span_iter = kSpanKindMap.find(span_kind);
  if (span_iter != kSpanKindMap.end()) {
    options_[FLUENT_FIELD_SPAN_KIND] = span_iter->second;
  }
}

void Recordable::SetInstrumentationScope(
    const opentelemetry::sdk::instrumentationscope::InstrumentationScope
        &instrumentation_scope) noexcept {
  options_["tags"]["otel.library.name"] = instrumentation_scope.GetName();
  options_["tags"]["otel.library.version"] =
      instrumentation_scope.GetVersion();
}

} // namespace trace
} // namespace fluentd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
