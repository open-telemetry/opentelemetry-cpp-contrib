#include "post_span_processor.h"

void PostSpanProcessor::OnEnd(std::unique_ptr<opentelemetry::sdk::trace::Recordable> &&span) noexcept
{
  ProxyRecordable* proxy = static_cast<ProxyRecordable*>(span.get());
  SimpleSpanProcessor::OnEnd(std::move(proxy->GetRealRecordable()));
}

