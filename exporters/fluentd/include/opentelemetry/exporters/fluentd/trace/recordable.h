// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "nlohmann/json.hpp"
#include "opentelemetry/sdk/trace/recordable.h"
#include "opentelemetry/version.h"

#include "opentelemetry/exporters/fluentd/common/fluentd_fields.h"

#include <chrono>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace fluentd {
namespace trace {
using FluentdSpan = nlohmann::json;

static inline nlohmann::byte_container_with_subtype<std::vector<std::uint8_t>>
get_msgpack_eventtimeext(int32_t seconds = 0, int32_t nanoseconds = 0) {
  if ((seconds == 0) && (nanoseconds == 0)) {
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    auto duration = tp.time_since_epoch();
    seconds = static_cast<int32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(duration).count());
    nanoseconds = static_cast<int32_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() %
        1000000000);
  }
  nlohmann::byte_container_with_subtype<std::vector<std::uint8_t>> ts{
      std::vector<uint8_t>{0, 0, 0, 0, 0, 0, 0, 0}};
  for (int i = 3; i >= 0; i--) {
    ts[i] = seconds & 0xff;
    ts[i + 4] = nanoseconds & 0xff;
    seconds >>= 8;
    nanoseconds >>= 8;
  }
  ts.set_subtype(0x00);
  return ts;
}

class Recordable final : public sdk::trace::Recordable {
public:
  Recordable(std::string tag = FLUENT_VALUE_SPAN) : sdk::trace::Recordable() {
    tag_ = tag;
    events_ = nlohmann::json::array();
  }

  const FluentdSpan span() const noexcept {
    FluentdSpan result;
    result["tag"] = tag_;
    result["events"] = events_;
    if (options_.size()) {
      result["options"] = options_;
    }
    return result;
  }

  void
  SetIdentity(const opentelemetry::trace::SpanContext &span_context,
              opentelemetry::trace::SpanId parent_span_id) noexcept override;

  void SetAttribute(
      nostd::string_view key,
      const opentelemetry::common::AttributeValue &value) noexcept override;

  void AddEvent(nostd::string_view name,
                opentelemetry::common::SystemTimestamp timestamp,
                const opentelemetry::common::KeyValueIterable
                    &attributes) noexcept override;

  void AddLink(const opentelemetry::trace::SpanContext &span_context,
               const opentelemetry::common::KeyValueIterable
                   &attributes) noexcept override;

  void SetStatus(opentelemetry::trace::StatusCode code,
                 nostd::string_view description) noexcept override;

  void SetName(nostd::string_view name) noexcept override;

  void SetStartTime(
      opentelemetry::common::SystemTimestamp start_time) noexcept override;

  void SetSpanKind(opentelemetry::trace::SpanKind span_kind) noexcept override;

  void SetResource(
      const opentelemetry::sdk::resource::Resource &resource) noexcept override;

  void SetDuration(std::chrono::nanoseconds duration) noexcept override;

  void SetInstrumentationLibrary(
      const opentelemetry::sdk::instrumentationlibrary::InstrumentationLibrary
          &instrumentation_library) noexcept override;

private:
  std::string tag_;
  nlohmann::json events_;
  nlohmann::json options_;
};

} // namespace trace
} // namespace fluentd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
