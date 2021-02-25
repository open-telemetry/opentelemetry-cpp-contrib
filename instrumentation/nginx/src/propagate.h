#pragma once

#include "trace_context.h"
#include <opentelemetry/context/context.h>

struct OtelCarrier {
  ngx_http_request_t* req;
  TraceContext* traceContext;
};

opentelemetry::context::Context ExtractContext(OtelCarrier* carrier);
void InjectContext(OtelCarrier* carrier, opentelemetry::context::Context context);
opentelemetry::trace::SpanContext GetCurrentSpan(opentelemetry::context::Context context);
