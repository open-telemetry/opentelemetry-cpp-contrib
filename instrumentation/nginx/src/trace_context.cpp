#include "trace_context.h"
#include "nginx_utils.h"

static TraceHeader*
FindEmptyOrExistingSlot(TraceContext* context, opentelemetry::nostd::string_view traceType) {
  for (TraceHeader& slot : context->traceHeader) {
    if (slot.key.len == 0) {
      return &slot;
    }

    if (slot.key.len == traceType.size() && ngx_strcmp(slot.key.data, traceType.data()) == 0) {
      return &slot;
    }
  }
  return nullptr;
}

static bool IsEmpty(ngx_str_t s) { return s.len == 0; }

ngx_str_t CreatePooledString(ngx_pool_t* pool, opentelemetry::nostd::string_view value) {
  u_char* data = (u_char*)ngx_palloc(pool, value.size());

  if (!data) {
    return ngx_null_string;
  }

  ngx_memcpy(data, value.data(), value.size());

  return {value.size(), data};
}

bool TraceContextSetTraceHeader(
  TraceContext* context, opentelemetry::nostd::string_view traceType,
  opentelemetry::nostd::string_view traceValue) {

  if (traceType.empty()) {
    return false;
  }

  TraceHeader* slot = FindEmptyOrExistingSlot(context, traceType);

  if (!slot) {
    return false;
  }

  ngx_pool_t* pool = context->request->pool;
  ngx_str_t key = CreatePooledString(pool, traceType);

  if (IsEmpty(key)) {
    return false;
  }

  ngx_str_t value = CreatePooledString(pool, traceValue);

  if (IsEmpty(value)) {
    return false;
  }

  slot->key = key;
  slot->value = value;
  return true;
}

const TraceHeader*
TraceContextFindTraceHeader(const TraceContext* context, opentelemetry::nostd::string_view key) {
  if (key.empty()) {
    return nullptr;
  }

  for (const TraceHeader& slot : context->traceHeader) {
    if (slot.key.len == key.size() && ngx_strncmp(slot.key.data, key.data(), slot.key.len) == 0) {
      return &slot;
    }
  }

  return nullptr;
}
