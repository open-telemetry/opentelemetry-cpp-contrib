// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#define ENABLE_LOGS_PREVIEW 1

#include "nlohmann/json.hpp"
#include "opentelemetry/sdk/common/attribute_utils.h"
#include "opentelemetry/sdk/logs/recordable.h"
#include "opentelemetry/version.h"

#include "opentelemetry/exporters/fluentd/common/fluentd_common.h"
#include "opentelemetry/exporters/fluentd/common/fluentd_fields.h"

#include <chrono>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace fluentd {
namespace logs {
using FluentdLog = nlohmann::json;

class Recordable final : public opentelemetry::sdk::logs::Recordable {
public:
  /**
   * Set the severity for this log.
   * @param severity the severity of the event
   */
  void SetSeverity(opentelemetry::logs::Severity severity) noexcept override;

  /**
   * Set name for this log
   * @param name the name to set
   */
  void SetName(nostd::string_view name) noexcept override;

  /**
   * Set body field for this log.
   * @param message the body to set
   */
  void SetBody(nostd::string_view message) noexcept override;

  /**
   * Set a resource for this log.
   * @param name the name of the resource
   * @param value the resource value
   */
  void SetResource(
      nostd::string_view key,
      const opentelemetry::common::AttributeValue &value) noexcept override {
  } // Not Supported

  /**
   * Set an attribute of a log.
   * @param key the key of the attribute
   * @param value the attribute value
   */
  void SetAttribute(
      nostd::string_view key,
      const opentelemetry::common::AttributeValue &value) noexcept override;

  /**
   * Set trace id for this log.
   * @param trace_id the trace id to set
   */
  void SetTraceId(opentelemetry::trace::TraceId trace_id) noexcept override;

  /**
   * Set span id for this log.
   * @param span_id the span id to set
   */
  virtual void
  SetSpanId(opentelemetry::trace::SpanId span_id) noexcept override;

  /**
   * Inject a trace_flags  for this log.
   * @param trace_flags the span id to set
   */
  void SetTraceFlags(
      opentelemetry::trace::TraceFlags trace_flags) noexcept override {
  } // Not Supported

  /**
   * Set the timestamp for this log.
   * @param timestamp the timestamp of the event
   */
  void SetTimestamp(
      opentelemetry::common::SystemTimestamp timestamp) noexcept override;

  nlohmann::json &Log() { return json_; }

private:
  nlohmann::json json_;
};

} // namespace logs
} // namespace fluentd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
