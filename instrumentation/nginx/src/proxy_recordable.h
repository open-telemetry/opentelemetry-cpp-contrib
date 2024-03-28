#ifndef OPENTELEMETRY_NGINX_PROXY_RECORDABLE_H
#define OPENTELEMETRY_NGINX_PROXY_RECORDABLE_H

#include <opentelemetry/exporters/otlp/otlp_recordable.h>

namespace trace = opentelemetry::trace;
namespace nostd = opentelemetry::nostd;
namespace sdktrace = opentelemetry::sdk::trace;

/**
 * Wraps actual recordable object so we could override span.kind through SetAttribute. This workarounds the problem that
 * in OpenTelemetry cpp SDK span kind is immutable on a span, but it causes problems for this instrumentation library.
 */
class ProxyRecordable : public sdktrace::Recordable {
public:

    ProxyRecordable()
            : _realRecordable(new opentelemetry::exporter::otlp::OtlpRecordable()) { }

    std::unique_ptr<opentelemetry::exporter::otlp::OtlpRecordable> GetRealRecordable() {
      return std::unique_ptr<opentelemetry::exporter::otlp::OtlpRecordable>(std::move(_realRecordable));
    }

    void SetIdentity(const opentelemetry::trace::SpanContext &span_context,
                     opentelemetry::trace::SpanId parent_span_id) noexcept override {
      _realRecordable->SetIdentity(span_context, parent_span_id);
    }

    virtual void SetAttribute(nostd::string_view key,
                              const opentelemetry::common::AttributeValue &value) noexcept override {
      if (key == "span.kind") {
        trace::SpanKind kind = static_cast<trace::SpanKind>(nostd::get<int>(value));
        _realRecordable->SetSpanKind(kind);
      } else {
        _realRecordable->SetAttribute(key, value);
      }
    }

    void AddEvent(nostd::string_view name,
                  opentelemetry::common::SystemTimestamp timestamp,
                  const opentelemetry::common::KeyValueIterable &attributes) noexcept override {
      _realRecordable->AddEvent(name, timestamp, attributes);
    };

    void AddLink(const opentelemetry::trace::SpanContext &span_context,
                 const opentelemetry::common::KeyValueIterable &attributes) noexcept override {
      _realRecordable->AddLink(span_context, attributes);
    };

    void SetStatus(opentelemetry::trace::StatusCode code,
                   nostd::string_view description) noexcept override {
      _realRecordable->SetStatus(code, description);
    };

    void SetName(nostd::string_view name) noexcept override {
      _realRecordable->SetName(name);
    };

    void SetSpanKind(opentelemetry::trace::SpanKind span_kind) noexcept override {
      _realRecordable->SetSpanKind(span_kind);
    };

    void SetResource(const opentelemetry::sdk::resource::Resource &resource) noexcept override {
      _realRecordable->SetResource(resource);
    };

    void SetStartTime(opentelemetry::common::SystemTimestamp start_time) noexcept override {
      _realRecordable->SetStartTime(start_time);
    };

    void SetDuration(std::chrono::nanoseconds duration) noexcept override {
      _realRecordable->SetDuration(duration);
    };

    void SetInstrumentationScope(const sdktrace::InstrumentationScope &instrumentation_scope) noexcept override {
      _realRecordable->SetInstrumentationScope(instrumentation_scope);
    };

private:
    std::unique_ptr<opentelemetry::exporter::otlp::OtlpRecordable> _realRecordable;
};

#endif //OPENTELEMETRY_NGINX_PROXY_RECORDABLE_H
