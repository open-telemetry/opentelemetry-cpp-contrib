#ifndef OPENTELEMETRY_NGINX_POST_SPAN_PROCESSOR_H
#define OPENTELEMETRY_NGINX_POST_SPAN_PROCESSOR_H

#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include "proxy_recordable.h"

namespace trace = opentelemetry::trace;
namespace nostd = opentelemetry::nostd;
namespace sdktrace = opentelemetry::sdk::trace;

class PostSpanProcessor : public sdktrace::SimpleSpanProcessor {
public:

    explicit PostSpanProcessor(std::unique_ptr<sdktrace::SpanExporter> &&exporter) noexcept
    : SimpleSpanProcessor(std::move(exporter))
    {}

    void OnEnd(std::unique_ptr<opentelemetry::sdk::trace::Recordable> &&span) noexcept override;

    std::unique_ptr<opentelemetry::sdk::trace::Recordable> MakeRecordable() noexcept override {
      return std::unique_ptr<ProxyRecordable>(new ProxyRecordable());
    }
};

#endif //OPENTELEMETRY_NGINX_POST_SPAN_PROCESSOR_H
