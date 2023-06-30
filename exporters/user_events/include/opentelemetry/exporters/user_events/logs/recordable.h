// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef ENABLE_LOGS_PREVIEW

#  include "opentelemetry/nostd/string_view.h"
#  include "opentelemetry/logs/severity.h"
#  include "opentelemetry/sdk/common/attribute_utils.h"
#  include "opentelemetry/sdk/logs/recordable.h"
#  include "opentelemetry/version.h"
#  include "utils.h"

#  include <eventheader/EventHeaderDynamic.h>
#  include <chrono>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace user_events
{
namespace logs
{

class Recordable final : public opentelemetry::sdk::logs::Recordable
{
public:
  ehd::EventBuilder &GetEventBuilder() noexcept { return event_builder_; }

  int GetLevelIndex() noexcept { return level_index_; }

  bool PrepareExport() noexcept;

  /**
   * Construct a new Recordable object
   *
   */
  Recordable() noexcept;
  /**
   * Set the severity for this log.
   * @param severity the severity of the event
   */
  void SetSeverity(opentelemetry::logs::Severity severity) noexcept override;

  /**
   * Set name for this log
   * @param name the name to set
   */
  void SetName(nostd::string_view name) noexcept;

  /**
   * Set body field for this log.
   * @param message the body to set
   */
  void SetBody(const opentelemetry::common::AttributeValue &message) noexcept override;

  /**
   * Set event id for this log.
   * @param id the id to set
   * @param name the name to set
   */
  void SetEventId(int64_t id, nostd::string_view name) noexcept override;

  /**
   * Set a resource for this log.
   * @param Resource the resource to set
   */
  void SetResource(const opentelemetry::sdk::resource::Resource &resource) noexcept override {
  }  // Not Supported

  /**
   * Set an attribute of a log.
   * @param key the key of the attribute
   * @param value the attribute value
   */
  void SetAttribute(nostd::string_view key,
                    const opentelemetry::common::AttributeValue &value) noexcept override;

  /**
   * Set trace id for this log.
   * @param trace_id the trace id to set
   */
  void SetTraceId(const opentelemetry::trace::TraceId &trace_id) noexcept override;

  /**
   * Set span id for this log.
   * @param span_id the span id to set
   */
  void SetSpanId(const opentelemetry::trace::SpanId &span_id) noexcept override;

  /**
   * Inject a trace_flags  for this log.
   * @param trace_flags the span id to set
   */
  void SetTraceFlags(const opentelemetry::trace::TraceFlags &trace_flags) noexcept override {
  }  // Not Supported

  /**
   * Set the timestamp for this log.
   * @param timestamp the timestamp of the event
   */
  void SetTimestamp(opentelemetry::common::SystemTimestamp timestamp) noexcept override;

  /**
   * Set the observed timestamp for this log.
   * @param timestamp the timestamp to set
   */
  void SetObservedTimestamp(common::SystemTimestamp timestamp) noexcept override {}

  /**
   * Set instrumentation_scope for this log.
   * @param instrumentation_scope the instrumentation scope to set
   */
  void SetInstrumentationScope(const opentelemetry::sdk::instrumentationscope::InstrumentationScope
                                   &instrumentation_scope) noexcept override
  {}  // Not Supported

private:
  ehd::EventBuilder event_builder_;
  int level_index_;
  size_t cs_part_b_bookmark_      = 0;
  size_t cs_part_b_bookmark_size_ = 0;
  size_t cs_part_c_bookmark_      = 0;
  size_t cs_part_c_bookmark_size_ = 0;
};

}  // namespace logs
}  // namespace user_events
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE

#endif  // ENABLE_LOGS_PREVIEW
