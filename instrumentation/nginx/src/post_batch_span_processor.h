#ifndef OPENTELEMETRY_NGINX_POST_BATCH_SPAN_PROCESSOR_H
#define OPENTELEMETRY_NGINX_POST_BATCH_SPAN_PROCESSOR_H

#include "proxy_recordable.h"
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>

namespace trace = opentelemetry::trace;
namespace nostd = opentelemetry::nostd;
namespace sdktrace = opentelemetry::sdk::trace;

class PostBatchSpanProcessor : public sdktrace::BatchSpanProcessor {
public:

    explicit PostBatchSpanProcessor(std::unique_ptr<sdktrace::SpanExporter> &&exporter,
                                    const sdktrace::BatchSpanProcessorOptions &options) noexcept
            : BatchSpanProcessor(std::move(exporter), options)
    {}

    void OnEnd(std::unique_ptr<opentelemetry::sdk::trace::Recordable> &&span) noexcept override;

    std::unique_ptr<opentelemetry::sdk::trace::Recordable> MakeRecordable() noexcept override {
      return std::unique_ptr<ProxyRecordable>(new ProxyRecordable());
    }
};

#endif //OPENTELEMETRY_NGINX_POST_BATCH_SPAN_PROCESSOR_H
