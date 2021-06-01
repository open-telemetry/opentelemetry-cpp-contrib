#include "propagate.h"
#include "location_config.h"
#include "nginx_utils.h"
#include <opentelemetry/context/context_value.h>
#include <opentelemetry/nostd/mpark/variant.h>
#include <opentelemetry/trace/propagation/b3_propagator.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/trace/span.h>

namespace trace = opentelemetry::trace;
namespace nostd = opentelemetry::nostd;

using OtelB3Propagator = trace::propagation::B3Propagator;
using OtelB3MultiPropagator = trace::propagation::B3PropagatorMultiHeader;
using OtelW3CPropagator = trace::propagation::HttpTraceContext;

static bool FindHeader(ngx_http_request_t* req, nostd::string_view key, nostd::string_view* value) {
  ngx_list_part_t* part = &req->headers_in.headers.part;
  ngx_table_elt_t* h = (ngx_table_elt_t*)part->elts;

  for (ngx_uint_t i = 0;; i++) {
    if (i >= part->nelts) {
      if (part->next == nullptr) {
        break;
      }

      part = part->next;
      h = (ngx_table_elt_t*)part->elts;
      i = 0;
    }

    if (
      key.size() != h[i].key.len ||
      ngx_strncasecmp((u_char*)key.data(), h[i].key.data, key.length()) != 0) {
      continue;
    }

    *value = FromNgxString(h[i].value);
    return true;
  }

  return false;
}


static bool HasHeader(ngx_http_request_t* req, nostd::string_view header) {
  nostd::string_view value;
  return FindHeader(req, header, &value);
}

class TextMapCarrierNgx : public opentelemetry::context::propagation::TextMapCarrier
{
public:
  TextMapCarrierNgx(OtelCarrier* carrier) : carrier_(carrier) {}
  virtual nostd::string_view Get(nostd::string_view key) const noexcept override
  {
    nostd::string_view value;
    FindHeader(carrier_->req, key, &value);
    return value;
  }
  virtual void Set(nostd::string_view key, nostd::string_view value) noexcept override
  {
    TraceContextSetTraceHeader(carrier_->traceContext, key, value);
  }

  OtelCarrier* carrier_;
};

TracePropagationType GetPropagationType(ngx_http_request_t* req) {
  OtelNgxLocationConf* config = GetOtelLocationConf(req);
  return config->propagationType;
}

opentelemetry::context::Context ExtractContext(OtelCarrier* carrier) {
  TracePropagationType propagationType = GetPropagationType(carrier->req);
  TextMapCarrierNgx textMapCarrier(carrier);

  opentelemetry::context::Context root;
  switch (propagationType) {
    case TracePropagationW3C: {
      return OtelW3CPropagator().Extract(textMapCarrier, root);
    }
    case TracePropagationB3: {
      if (HasHeader(carrier->req, "b3")) {
        return OtelB3Propagator().Extract(textMapCarrier, root);
      }

      return OtelB3MultiPropagator().Extract(textMapCarrier, root);
    }
    default:
      return root;
  }
}

void InjectContext(OtelCarrier* carrier, opentelemetry::context::Context context) {
  TracePropagationType propagationType = GetPropagationType(carrier->req);
  TextMapCarrierNgx textMapCarrier(carrier);

  switch (propagationType) {
    case TracePropagationW3C: {
      OtelW3CPropagator().Inject(textMapCarrier, context);
      break;
    }
    case TracePropagationB3: {
      OtelB3Propagator().Inject(textMapCarrier, context);
      break;
    }
    default:
      break;
  }
}

opentelemetry::trace::SpanContext GetCurrentSpan(opentelemetry::context::Context context) {
  opentelemetry::context::ContextValue span = context.GetValue(trace::kSpanKey);
  if (nostd::holds_alternative<nostd::shared_ptr<trace::Span>>(span)) {
    return nostd::get<opentelemetry::nostd::shared_ptr<trace::Span>>(span).get()->GetContext();
  }

  return trace::SpanContext::GetInvalid();
}
