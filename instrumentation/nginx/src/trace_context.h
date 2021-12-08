#pragma once

#include <opentelemetry/trace/span.h>
#include <unordered_map>
#include "script.h"

extern "C" {
#include <ngx_http.h>
}

struct TraceHeader {
  ngx_str_t key = ngx_null_string;
  ngx_str_t value = ngx_null_string;
};

enum TracePropagationType {
  TracePropagationUnset,
  TracePropagationW3C,
  TracePropagationB3,
};

struct TraceContext {
  TraceContext(ngx_http_request_t* req) : request(req), traceHeader{} {}
  /* The current request being handled by nginx. */
  ngx_http_request_t* request;
  opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> request_span;
  /* Headers to be injected for the upstream request. */
  TraceHeader traceHeader[2];
};

bool TraceContextSetTraceHeader(
  TraceContext* context, opentelemetry::nostd::string_view key,
  opentelemetry::nostd::string_view value);

const TraceHeader*
TraceContextFindTraceHeader(const TraceContext* context, opentelemetry::nostd::string_view key);
