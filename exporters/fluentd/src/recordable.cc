/// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/fluentd/recordable.h"
#include "opentelemetry/exporters/fluentd/fluentd_logging.h"


#include <map>
#include <string>

using namespace nlohmann;

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace fluentd
{
template<typename T>
static inline json create_message(T ts, json body)
{
  auto arr = json::array();
  arr.push_back(ts);
  arr.push_back(body);
  return arr;
}

// constexpr needs keys to be constexpr, const is next best to use.
static const std::map<opentelemetry::trace::SpanKind, std::string> kSpanKindMap = {
    {opentelemetry::trace::SpanKind::kClient, "CLIENT"},
    {opentelemetry::trace::SpanKind::kServer, "SERVER"},
    {opentelemetry::trace::SpanKind::kConsumer, "CONSUMER"},
    {opentelemetry::trace::SpanKind::kProducer, "PRODUCER"},
};

//
// See `attribute_value.h` for details.
//
const int kAttributeValueSize = 15;

void Recordable::SetIdentity(const opentelemetry::trace::SpanContext &span_context,
                             opentelemetry::trace::SpanId parent_span_id) noexcept
{
  LOG_DEBUG("LALIT -> Set Identity");
  char trace_id_lower_base16[trace::TraceId::kSize * 2] = {0};
  span_context.trace_id().ToLowerBase16(trace_id_lower_base16);
  char span_id_lower_base16[trace::SpanId::kSize * 2] = {0};
  span_context.span_id().ToLowerBase16(span_id_lower_base16);
  char parent_span_id_lower_base16[trace::SpanId::kSize * 2] = {0};
  parent_span_id.ToLowerBase16(parent_span_id_lower_base16);
  options_[FLUENT_FIELD_SPAN_ID] = std::string(span_id_lower_base16, 16);
  options_[FLUENT_FIELD_SPAN_PARENTID] = std::string(parent_span_id_lower_base16, 16);
  options_[FLUENT_FIELD_TRACE_ID] = std::string(trace_id_lower_base16, 32);
}

void PopulateAttribute(nlohmann::json &attribute,
                       nostd::string_view key,
                       const opentelemetry::common::AttributeValue &value)
{
  // Assert size of variant to ensure that this method gets updated if the variant
  // definition changes
  static_assert(
      nostd::variant_size<opentelemetry::common::AttributeValue>::value == kAttributeValueSize+1,
      "AttributeValue contains unknown type");

  if (nostd::holds_alternative<bool>(value))
  {
    attribute[key.data()] = nostd::get<bool>(value);
  }
  else if (nostd::holds_alternative<int>(value))
  {
    attribute[key.data()] = nostd::get<int>(value);
  }
  else if (nostd::holds_alternative<int64_t>(value))
  {
    attribute[key.data()] = nostd::get<int64_t>(value);
  }
  else if (nostd::holds_alternative<unsigned int>(value))
  {
    attribute[key.data()] = nostd::get<unsigned int>(value);
  }
  else if (nostd::holds_alternative<uint64_t>(value))
  {
    attribute[key.data()] = nostd::get<uint64_t>(value);
  }
  else if (nostd::holds_alternative<double>(value))
  {
    attribute[key.data()] = nostd::get<double>(value);
  }
  else if (nostd::holds_alternative<nostd::string_view>(value))
  {
    LOG_DEBUG( " LALIT -> Setting string attribute env_properties");
    attribute[key.data()] =
        std::string(nostd::get<nostd::string_view>(value).data());
        // nostd::string_view(nostd::get<nostd::string_view>(value).data(),
        // nostd::get<nostd::string_view>(value).size());
  }
  else if (nostd::holds_alternative<nostd::span<const uint8_t>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const uint8_t>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const bool>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const bool>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const int>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const int>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const int64_t>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const int64_t>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const unsigned int>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const unsigned int>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const uint64_t>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const uint64_t>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const double>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const double>>(value))
    {
      attribute[key.data()].push_back(val);
    }
  }
  else if (nostd::holds_alternative<nostd::span<const nostd::string_view>>(value))
  {
    attribute[key.data()] = {};
    for (const auto &val : nostd::get<nostd::span<const nostd::string_view>>(value))
    {
      attribute[key.data()].push_back(std::string(val.data(), val.size()));
    }
  }
}

void Recordable::SetAttribute(nostd::string_view key,
                              const opentelemetry::common::AttributeValue &value) noexcept
{
  LOG_DEBUG( " LALIT:Setting env_properties");
  if (!options_.contains(FLUENT_FIELD_PROPERTIES))
  {
    options_[FLUENT_FIELD_PROPERTIES] = nlohmann::json::object();
  }
  
  PopulateAttribute(options_[FLUENT_FIELD_PROPERTIES], key, value);
}

void Recordable::AddEvent(nostd::string_view name,
                          common::SystemTimestamp timestamp,
                          const common::KeyValueIterable &attributes) noexcept
{
  LOG_DEBUG("Add event : ");
  nlohmann::json attrs = nlohmann::json::object();  // empty object
  attributes.ForEachKeyValue([&](nostd::string_view key, common::AttributeValue value) noexcept {
    PopulateAttribute(attrs, key, value);
    return true;
  });

  // Event name
  attrs[FLUENT_FIELD_NAME] = name;
  auto ts       = get_msgpack_eventtimeext();  
  events_.push_back(create_message(ts, attrs));
}

void Recordable::AddLink(const opentelemetry::trace::SpanContext &span_context,
                         const common::KeyValueIterable &attributes) noexcept
{
  // TODO: Currently not supported by specs:
  // https://github.com/open-telemetry/opentelemetry-specification/blob/master/specification/trace/sdk_exporters/fluentd.md
}

void Recordable::SetStatus(trace::StatusCode code, nostd::string_view description) noexcept
{
  LOG_DEBUG("LALIT -> SetStatus");
  if (code != trace::StatusCode::kUnset)
  {
    options_["tags"]["otel.status_code"] = code;
    if (code == trace::StatusCode::kError)
    {
      options_["tags"]["error"] = description;
    }
  }
}

void Recordable::SetName(nostd::string_view name) noexcept
{
  // Span name.. Should this be tag name?
  LOG_DEBUG("LALIT - SetName");
  options_[FLUENT_FIELD_NAME] = name.data();
}

void Recordable::SetResource(const opentelemetry::sdk::resource::Resource &resource) noexcept
{
  LOG_DEBUG("LALIT -> SetResource");
  // only tag attribute is supported by specs as of now.
  auto attributes = resource.GetAttributes();
  if (attributes.find("tag") != attributes.end())
  {
    tag_ = nostd::get<std::string>(attributes["tag"]);
  }
}

void Recordable::SetStartTime(opentelemetry::common::SystemTimestamp start_time) noexcept
{
   LOG_DEBUG("LALIT -> SetStartTime");
  options_[FLUENT_FIELD_STARTTIME] =
      get_msgpack_eventtimeext(
        static_cast<int32_t>(std::chrono::duration_cast<std::chrono::seconds>(start_time.time_since_epoch()).count()),
        std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count() % 1000000000);
}

void Recordable::SetDuration(std::chrono::nanoseconds duration) noexcept
{
  LOG_DEBUG("SetDuration");
  options_[FLUENT_FIELD_ENDTTIME] = get_msgpack_eventtimeext();
  options_[FLUENT_FIELD_DURATION] = duration.count();
}

void Recordable::SetSpanKind(opentelemetry::trace::SpanKind span_kind) noexcept
{
  LOG_DEBUG("SetSpanKind");
  auto span_iter = kSpanKindMap.find(span_kind);
  if (span_iter != kSpanKindMap.end())
  {
    options_[FLUENT_FIELD_SPAN_KIND] = span_iter->second;
  }
}

void Recordable::SetInstrumentationLibrary(
    const opentelemetry::sdk::instrumentationlibrary::InstrumentationLibrary
        &instrumentation_library) noexcept
{
  LOG_DEBUG("SetInstrumentationLibrary");
  options_["tags"]["otel.library.name"]    = instrumentation_library.GetName();
  options_["tags"]["otel.library.version"] = instrumentation_library.GetVersion();
}

}  // namespace fluentd
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
