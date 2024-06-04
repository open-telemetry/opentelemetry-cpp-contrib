#include "post_batch_span_processor.h"
#include <opentelemetry/exporters/otlp/otlp_recordable.h>

void PostBatchSpanProcessor::OnEnd(std::unique_ptr<opentelemetry::sdk::trace::Recordable> &&span) noexcept
{
  ProxyRecordable* proxy = static_cast<ProxyRecordable*>(span.get());
  BatchSpanProcessor::OnEnd(std::move(proxy->GetRealRecordable()));
}
