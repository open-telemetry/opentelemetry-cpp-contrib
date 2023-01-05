// otlp_grpc_exporter header has to be included before any other API header to 
// avoid conflict between Abseil library and OpenTelemetry C++ absl::variant.
// https://github.com/open-telemetry/opentelemetry-cpp/tree/main/examples/otlp#additional-notes-regarding-abseil-library
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter.h>

#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/trace/span.h>
#include <algorithm>
#include <unordered_map>
#include <vector>

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

extern ngx_module_t otel_ngx_module;
}

#include "agent_config.h"
#include "location_config.h"
#include "nginx_config.h"
#include "nginx_utils.h"
#include "propagate.h"
#include <opentelemetry/context/context.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>
#include <opentelemetry/sdk/trace/id_generator.h>
#include <opentelemetry/sdk/trace/samplers/always_off.h>
#include <opentelemetry/sdk/trace/samplers/always_on.h>
#include <opentelemetry/sdk/trace/samplers/parent.h>
#include <opentelemetry/sdk/trace/samplers/trace_id_ratio.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/trace/provider.h>

namespace trace = opentelemetry::trace;
namespace nostd = opentelemetry::nostd;
namespace sdktrace = opentelemetry::sdk::trace;
namespace otlp = opentelemetry::exporter::otlp;

constexpr char kOtelCtxVarPrefix[] = "opentelemetry_context_";

const ScriptAttributeDeclaration kDefaultScriptAttributes[] = {
  {"http.scheme", "$scheme"},
  {"net.host.port", "$server_port", ScriptAttributeInt},
  {"net.peer.ip", "$remote_addr"},
  {"net.peer.port", "$remote_port", ScriptAttributeInt},
};

struct OtelMainConf {
  ngx_array_t* scriptAttributes;
  OtelNgxAgentConfig agentConfig;
};

nostd::shared_ptr<trace::Tracer> GetTracer() {
  return trace::Provider::GetTracerProvider()->GetTracer("nginx");
}

nostd::string_view NgxHttpFlavor(ngx_http_request_t* req) {
  switch (req->http_version) {
    case NGX_HTTP_VERSION_11:
      return "1.1";
    case NGX_HTTP_VERSION_20:
      return "2.0";
    case NGX_HTTP_VERSION_10:
      return "1.0";
    default:
      return "";
  }
}

static void NgxNormalizeAndCopyString(u_char* dst, ngx_str_t str) {
  for (ngx_uint_t i = 0; i < str.len; ++i) {
    u_char ch = str.data[i];
    if (ch >= 'A' && ch <= 'Z') {
      ch += 0x20;
    } else if (ch == '-') {
      ch = '_';
    }

    dst[i] = ch;
  }
}

static void OtelCaptureHeaders(nostd::shared_ptr<opentelemetry::trace::Span> span, ngx_str_t keyPrefix, ngx_list_t *headers,
#if (NGX_PCRE)
                        ngx_regex_t *sensitiveHeaderNames, ngx_regex_t *sensitiveHeaderValues,
#endif
                        nostd::span<ngx_table_elt_t*> excludedHeaders) {
  for (ngx_list_part_t *part = &headers->part; part != nullptr; part = part->next) {
    ngx_table_elt_t *header = (ngx_table_elt_t*) part->elts;
    for (ngx_uint_t i = 0; i < part->nelts; ++i) {
      if (std::find(excludedHeaders.begin(), excludedHeaders.end(), &header[i]) != excludedHeaders.end()) {
        continue;
      }

      u_char key[keyPrefix.len + header[i].key.len]; 
      NgxNormalizeAndCopyString((u_char*)ngx_copy(key, keyPrefix.data, keyPrefix.len), header[i].key);

      bool sensitiveHeader = false;
#if (NGX_PCRE)
      if (sensitiveHeaderNames) {
        int ovector[3];
        if (ngx_regex_exec(sensitiveHeaderNames, &header[i].key, ovector, 0) >= 0) {
          sensitiveHeader = true;
        }
      }
      if (sensitiveHeaderValues && !sensitiveHeader) {
        int ovector[3];
        if (ngx_regex_exec(sensitiveHeaderValues, &header[i].value, ovector, 0) >= 0) {
          sensitiveHeader = true;
        }
      }
#endif

      nostd::string_view value;
      if (sensitiveHeader) {
        value = "[REDACTED]";
      } else {
        value = FromNgxString(header[i].value);
      }

      span->SetAttribute({(const char*)key, keyPrefix.len + header[i].key.len}, nostd::span<const nostd::string_view>(&value, 1));
    }
  }
}

static ngx_int_t OtelGetContextVar(ngx_http_request_t*, ngx_http_variable_value_t*, uintptr_t) {
  // Filled out on context creation.
  return NGX_OK;
}

static ngx_int_t
OtelGetTraceContextVar(ngx_http_request_t* req, ngx_http_variable_value_t* v, uintptr_t data);

static ngx_int_t
OtelGetTraceId(ngx_http_request_t* req, ngx_http_variable_value_t* v, uintptr_t data);

static ngx_int_t
OtelGetSpanId(ngx_http_request_t* req, ngx_http_variable_value_t* v, uintptr_t data);

static ngx_http_variable_t otel_ngx_variables[] = {
  {
    ngx_string("otel_ctx"),
    nullptr,
    OtelGetContextVar,
    0,
    NGX_HTTP_VAR_NOCACHEABLE | NGX_HTTP_VAR_NOHASH,
    0,
  },
  {
    ngx_string(kOtelCtxVarPrefix),
    nullptr,
    OtelGetTraceContextVar,
    0,
    NGX_HTTP_VAR_PREFIX | NGX_HTTP_VAR_NOHASH | NGX_HTTP_VAR_NOCACHEABLE,
    0,
  },
  {
    ngx_string("opentelemetry_trace_id"),
    nullptr,
    OtelGetTraceId,
    0,
    NGX_HTTP_VAR_NOCACHEABLE | NGX_HTTP_VAR_NOHASH,
    0,
  },
  {
    ngx_string("opentelemetry_span_id"),
    nullptr,
    OtelGetSpanId,
    0,
    NGX_HTTP_VAR_NOCACHEABLE | NGX_HTTP_VAR_NOHASH,
    0,
  },
  ngx_http_null_variable,
};

static bool IsOtelEnabled(ngx_http_request_t* req) {
  OtelNgxLocationConf* locConf = GetOtelLocationConf(req);
  if (locConf->enabled) {
#if (NGX_PCRE)
    int ovector[3];
    return locConf->ignore_paths == nullptr || ngx_regex_exec(locConf->ignore_paths, &req->unparsed_uri, ovector, 0) < 0;
#else
    return true;
#endif
  } else {
    return false;
  }
}

TraceContext* GetTraceContext(ngx_http_request_t* req) {
  ngx_http_variable_value_t* val = ngx_http_get_indexed_variable(req, otel_ngx_variables[0].index);

  if (val == nullptr || val->not_found) {
    ngx_log_error(NGX_LOG_INFO, req->connection->log, 0, "TraceContext not found");
    return nullptr;
  }

  std::unordered_map<ngx_http_request_t*, TraceContext*>* map = (std::unordered_map<ngx_http_request_t*, TraceContext*>*)val->data;
  if (map == nullptr){
    ngx_log_error(NGX_LOG_INFO, req->connection->log, 0, "TraceContext not found");
    return nullptr;
  }
  auto it = map->find(req);
  if (it != map->end()) {
    return it->second;
  }
  ngx_log_error(NGX_LOG_INFO, req->connection->log, 0, "TraceContext not found");
  return nullptr;
}

nostd::string_view WithoutOtelVarPrefix(ngx_str_t value) {
  const size_t prefixLength = sizeof(kOtelCtxVarPrefix) - 1;

  if (value.len <= prefixLength) {
    return "";
  }

  return {(const char*)value.data + prefixLength, value.len - prefixLength};
}

static ngx_int_t
OtelGetTraceContextVar(ngx_http_request_t* req, ngx_http_variable_value_t* v, uintptr_t data) {
  if (!IsOtelEnabled(req)) {
    v->valid = 0;
    v->not_found = 1;
    return NGX_OK;
  }

  TraceContext* traceContext = GetTraceContext(req);

  if (traceContext == nullptr || !traceContext->request_span) {
    ngx_log_error(
      NGX_LOG_INFO, req->connection->log, 0,
      "Unable to get trace context when expanding tracecontext var");
    return NGX_OK;
  }

  ngx_str_t* prefixedKey = (ngx_str_t*)data;

  nostd::string_view key = WithoutOtelVarPrefix(*prefixedKey);

  const TraceHeader* header = TraceContextFindTraceHeader(traceContext, key);

  if (header) {
    v->len = header->value.len;
    v->valid = 1;
    v->no_cacheable = 1;
    v->not_found = 0;
    v->data = header->value.data;
  } else {
    v->len = 0;
    v->valid = 0;
    v->not_found = 1;
    v->no_cacheable = 1;
    v->data = nullptr;
  }

  return NGX_OK;
}

static ngx_int_t
OtelGetTraceId(ngx_http_request_t* req, ngx_http_variable_value_t* v, uintptr_t data) {
  if (!IsOtelEnabled(req)) {
    v->valid = 0;
    v->not_found = 1;
    return NGX_OK;
  }

  TraceContext* traceContext = GetTraceContext(req);

  if (traceContext == nullptr || !traceContext->request_span) {
    ngx_log_error(
      NGX_LOG_INFO, req->connection->log, 0,
      "Unable to get trace context when getting trace id");
    return NGX_OK;
  }

  trace::SpanContext spanContext = traceContext->request_span->GetContext();

  if (spanContext.IsValid()) {
    constexpr int len = 2 * trace::TraceId::kSize;
    char* data = (char*)ngx_palloc(req->pool, len);

    if(!data) {
      ngx_log_error(
        NGX_LOG_ERR, req->connection->log, 0,
        "Unable to allocate memory for the trace id");

      v->len = 0;
      v->valid = 0;
      v->no_cacheable = 1;
      v->not_found = 0;
      v->data = nullptr;

      return NGX_OK;
    }

    spanContext.trace_id().ToLowerBase16(nostd::span<char, len>{data, len});

    v->len = len;
    v->valid = 1;
    v->no_cacheable = 1;
    v->not_found = 0;
    v->data = (u_char*)data;
  } else {
    v->len = 0;
    v->valid = 0;
    v->no_cacheable = 1;
    v->not_found = 1;
    v->data = nullptr;
  }

  return NGX_OK;
}

static ngx_int_t
OtelGetSpanId(ngx_http_request_t* req, ngx_http_variable_value_t* v, uintptr_t data) {
  if (!IsOtelEnabled(req)) {
    v->valid = 0;
    v->not_found = 1;
    return NGX_OK;
  }

  TraceContext* traceContext = GetTraceContext(req);

  if (traceContext == nullptr || !traceContext->request_span) {
    ngx_log_error(
      NGX_LOG_INFO, req->connection->log, 0,
      "Unable to get trace context when getting span id");
    return NGX_OK;
  }

  trace::SpanContext spanContext = traceContext->request_span->GetContext();

  if (spanContext.IsValid()) {
    constexpr int len = 2 * trace::SpanId::kSize;
    char* data = (char*)ngx_palloc(req->pool, len);

    if(!data) {
      ngx_log_error(
        NGX_LOG_ERR, req->connection->log, 0,
        "Unable to allocate memory for the span id");

      v->len = 0;
      v->valid = 0;
      v->no_cacheable = 1;
      v->not_found = 0;
      v->data = nullptr;

      return NGX_OK;
    }

    spanContext.span_id().ToLowerBase16(nostd::span<char, len>{data, len});

    v->len = len;
    v->valid = 1;
    v->no_cacheable = 1;
    v->not_found = 0;
    v->data = (u_char*)data;
  } else {
    v->len = 0;
    v->valid = 0;
    v->no_cacheable = 1;
    v->not_found = 1;
    v->data = nullptr;
  }

  return NGX_OK;
}

void TraceContextCleanup(void* data) {
  TraceContext* context = (TraceContext*)data;
  context->~TraceContext();
}

void RequestContextMapCleanup(void* data) {
  std::unordered_map<ngx_http_request_t*, TraceContext*>* map = (std::unordered_map<ngx_http_request_t*, TraceContext*>*)data;
  map->~unordered_map();
}

nostd::string_view GetOperationName(ngx_http_request_t* req) {
  OtelNgxLocationConf* locationConf = GetOtelLocationConf(req);

  ngx_str_t opName = ngx_null_string;
  if (locationConf->operationNameScript.Run(req, &opName)) {
    return FromNgxString(opName);
  }

  ngx_http_core_loc_conf_t* httpCoreLocationConf =
    (ngx_http_core_loc_conf_t*)ngx_http_get_module_loc_conf(req, ngx_http_core_module);

  if (httpCoreLocationConf) {
    return FromNgxString(httpCoreLocationConf->name);
  }

  return FromNgxString(opName);
}

ngx_http_core_main_conf_t* NgxHttpModuleMainConf(ngx_http_request_t* req) {
  return (ngx_http_core_main_conf_t*)ngx_http_get_module_main_conf(req, ngx_http_core_module);
}

OtelMainConf* GetOtelMainConf(ngx_http_request_t* req) {
  return (OtelMainConf*)ngx_http_get_module_main_conf(req, otel_ngx_module);
}

nostd::string_view GetNgxServerName(const ngx_http_request_t* req) {
  ngx_http_core_srv_conf_t* cscf =
    (ngx_http_core_srv_conf_t*)ngx_http_get_module_srv_conf(req, ngx_http_core_module);
  return FromNgxString(cscf->server_name);
}

TraceContext* CreateTraceContext(ngx_http_request_t* req, ngx_http_variable_value_t* val) {
  ngx_pool_cleanup_t* cleanup = ngx_pool_cleanup_add(req->pool, sizeof(TraceContext));
  TraceContext* context = (TraceContext*)cleanup->data;
  new (context) TraceContext(req);
  cleanup->handler = TraceContextCleanup;

  std::unordered_map<ngx_http_request_t*, TraceContext*>* map;
  if (req->parent && val->data) {
    // Subrequests will already have the map created so just retrieve it
    map = (std::unordered_map<ngx_http_request_t*, TraceContext*>*)val->data;
  } else {
    ngx_pool_cleanup_t* cleanup = ngx_pool_cleanup_add(req->pool, sizeof(std::unordered_map<ngx_http_request_t*, TraceContext*>));
    map = (std::unordered_map<ngx_http_request_t*, TraceContext*>*)cleanup->data;
    new (map) std::unordered_map<ngx_http_request_t*, TraceContext*>();
    cleanup->handler = RequestContextMapCleanup;
    val->data = (unsigned char*)cleanup->data;
    val->len = sizeof(std::unordered_map<ngx_http_request_t*, TraceContext*>);
  }
  map->insert({req, context});
  return context;
}

ngx_int_t StartNgxSpan(ngx_http_request_t* req) {
  if (!IsOtelEnabled(req)) {
    return NGX_DECLINED;
  }

  // Internal requests must be called from another location in nginx, that should already have a trace. Without this check, a call would generate an extra (unrelated) span without much information
  if (req->internal) {
    return NGX_DECLINED;
  }

  ngx_http_variable_value_t* val = ngx_http_get_indexed_variable(req, otel_ngx_variables[0].index);

  if (!val) {
    ngx_log_error(NGX_LOG_ERR, req->connection->log, 0, "Unable to find OpenTelemetry context");
    return NGX_DECLINED;
  }

  TraceContext* context = CreateTraceContext(req, val);

  OtelCarrier carrier{req, context};
  opentelemetry::context::Context incomingContext;

  OtelNgxLocationConf* locConf = GetOtelLocationConf(req);

  if (locConf->trustIncomingSpans) {
    incomingContext = ExtractContext(&carrier);
  }

  trace::StartSpanOptions startOpts;
  startOpts.kind = trace::SpanKind::kServer;
  startOpts.parent = GetCurrentSpan(incomingContext);

  context->request_span = GetTracer()->StartSpan(
    GetOperationName(req),
    {
      {"http.method", FromNgxString(req->method_name)},
      {"http.flavor", NgxHttpFlavor(req)},
      {"http.target", FromNgxString(req->unparsed_uri)},
    },
    startOpts);

  nostd::string_view serverName = GetNgxServerName(req);
  if (!serverName.empty()) {
    context->request_span->SetAttribute("http.server_name", serverName);
  }

  if (req->headers_in.host) {
    context->request_span->SetAttribute("http.host", FromNgxString(req->headers_in.host->value));
  }

  if (req->headers_in.user_agent) {
    context->request_span->SetAttribute("http.user_agent", FromNgxString(req->headers_in.user_agent->value));
  }

  if (locConf->captureHeaders) {
    ngx_table_elt_t* excludedHeaders[] = {req->headers_in.host, req->headers_in.user_agent};
    OtelCaptureHeaders(context->request_span, ngx_string("http.request.header."), &req->headers_in.headers,
#if (NGX_PCRE)
                       locConf->sensitiveHeaderNames, locConf->sensitiveHeaderValues,
#endif
                       {excludedHeaders, 2});
  }

  auto outgoingContext = incomingContext.SetValue(trace::kSpanKey, context->request_span);

  InjectContext(&carrier, outgoingContext);

  return NGX_DECLINED;
}

void AddScriptAttributes(
  trace::Span* span, const ngx_array_t* attributes, ngx_http_request_t* req) {

  if (!attributes) {
    return;
  }

  CompiledScriptAttribute* elements = (CompiledScriptAttribute*)attributes->elts;
  for (ngx_uint_t i = 0; i < attributes->nelts; i++) {
    CompiledScriptAttribute* attribute = &elements[i];
    ngx_str_t key = ngx_null_string;
    ngx_str_t value = ngx_null_string;

    if (attribute->key.Run(req, &key) && attribute->value.Run(req, &value)) {
      switch(attribute->type) {
        case ScriptAttributeInt:
          span->SetAttribute(FromNgxString(key), ngx_atoi(value.data, value.len));
          break;
        default:
          span->SetAttribute(FromNgxString(key), FromNgxString(value));
      }
    }
  }
}

ngx_int_t FinishNgxSpan(ngx_http_request_t* req) {
  if (!IsOtelEnabled(req)) {
    return NGX_DECLINED;
  }

  TraceContext* context = GetTraceContext(req);

  if (!context) {
    return NGX_DECLINED;
  }

  auto span = context->request_span;
  span->SetAttribute("http.status_code", req->headers_out.status);

  OtelNgxLocationConf* locConf = GetOtelLocationConf(req);

  if (locConf->captureHeaders) {
    OtelCaptureHeaders(span, ngx_string("http.response.header."), &req->headers_out.headers,
#if (NGX_PCRE)
                       locConf->sensitiveHeaderNames, locConf->sensitiveHeaderValues,
#endif
                       {});
  }

  AddScriptAttributes(span.get(), GetOtelMainConf(req)->scriptAttributes, req);
  AddScriptAttributes(span.get(), locConf->customAttributes, req);

  span->UpdateName(GetOperationName(req));

  span->End();
  return NGX_DECLINED;
}

static ngx_int_t InitModule(ngx_conf_t* conf) {
  ngx_http_core_main_conf_t* main_conf =
    (ngx_http_core_main_conf_t*)ngx_http_conf_get_module_main_conf(conf, ngx_http_core_module);

  struct PhaseHandler {
    ngx_http_phases phase;
    ngx_http_handler_pt handler;
  };

  const PhaseHandler handlers[] = {
    {NGX_HTTP_REWRITE_PHASE, StartNgxSpan},
    {NGX_HTTP_LOG_PHASE, FinishNgxSpan},
  };

  for (const PhaseHandler& ph : handlers) {
    ngx_http_handler_pt* ngx_handler =
      (ngx_http_handler_pt*)ngx_array_push(&main_conf->phases[ph.phase].handlers);

    if (ngx_handler == nullptr) {
      continue;
    }

    *ngx_handler = ph.handler;
  }

  OtelMainConf* otelMainConf =
    (OtelMainConf*)ngx_http_conf_get_module_main_conf(conf, otel_ngx_module);

  if (!otelMainConf) {
    return NGX_ERROR;
  }

  otelMainConf->scriptAttributes = ngx_array_create(
    conf->pool, sizeof(kDefaultScriptAttributes) / sizeof(kDefaultScriptAttributes[0]),
    sizeof(CompiledScriptAttribute));

  if (otelMainConf->scriptAttributes == nullptr) {
    return NGX_ERROR;
  }

  for (const auto& attrib : kDefaultScriptAttributes) {
    CompiledScriptAttribute* compiledAttrib =
      (CompiledScriptAttribute*)ngx_array_push(otelMainConf->scriptAttributes);

    if (compiledAttrib == nullptr) {
      return false;
    }

    new (compiledAttrib) CompiledScriptAttribute();

    if (!CompileScriptAttribute(conf, attrib, compiledAttrib)) {
      return NGX_ERROR;
    }
  }

  return NGX_OK;
}

static void* CreateOtelMainConf(ngx_conf_t* conf) {
  OtelMainConf* mainConf = (OtelMainConf*)ngx_pcalloc(conf->pool, sizeof(OtelMainConf));
  new (mainConf) OtelMainConf();

  return mainConf;
}

static void* CreateOtelLocConf(ngx_conf_t* conf) {
  OtelNgxLocationConf* locConf =
    (OtelNgxLocationConf*)ngx_pcalloc(conf->pool, sizeof(OtelNgxLocationConf));
  new (locConf) OtelNgxLocationConf();
  return locConf;
}

static char* MergeLocConf(ngx_conf_t*, void* parent, void* child) {
  OtelNgxLocationConf* prev = (OtelNgxLocationConf*)parent;
  OtelNgxLocationConf* conf = (OtelNgxLocationConf*)child;

  ngx_conf_merge_value(conf->enabled, prev->enabled, 1);

  if (conf->propagationType == TracePropagationUnset) {
    if (prev->propagationType != TracePropagationUnset) {
      conf->propagationType = prev->propagationType;
    } else {
      conf->propagationType = TracePropagationW3C;
    }
  }

  ngx_conf_merge_value(conf->trustIncomingSpans, prev->trustIncomingSpans, 1);

  ngx_conf_merge_value(conf->captureHeaders, prev->captureHeaders, 0);

#if (NGX_PCRE)
  ngx_conf_merge_ptr_value(conf->sensitiveHeaderNames, prev->sensitiveHeaderNames, nullptr);
  ngx_conf_merge_ptr_value(conf->sensitiveHeaderValues, prev->sensitiveHeaderValues, nullptr);

  ngx_conf_merge_ptr_value(conf->ignore_paths, prev->ignore_paths, nullptr);
#endif

  if (!prev->operationNameScript.IsEmpty() && conf->operationNameScript.IsEmpty()) {
    conf->operationNameScript = prev->operationNameScript;
  }

  if (prev->customAttributes && !conf->customAttributes) {
    conf->customAttributes = prev->customAttributes;
  } else if (prev->customAttributes && conf->customAttributes) {
    std::unordered_map<nostd::string_view, CompiledScriptAttribute> mergedAttributes;

    for (ngx_uint_t i = 0; i < prev->customAttributes->nelts; i++) {
      CompiledScriptAttribute* attrib =
        &((CompiledScriptAttribute*)prev->customAttributes->elts)[i];
      mergedAttributes[FromNgxString(attrib->key.pattern)] = *attrib;
    }

    for (ngx_uint_t i = 0; i < conf->customAttributes->nelts; i++) {
      CompiledScriptAttribute* attrib =
        &((CompiledScriptAttribute*)conf->customAttributes->elts)[i];
      mergedAttributes[FromNgxString(attrib->key.pattern)] = *attrib;
    }

    ngx_uint_t index = 0;
    for (const auto& kv : mergedAttributes) {
      if (index == conf->customAttributes->nelts) {
        CompiledScriptAttribute* attribute =
          (CompiledScriptAttribute*)ngx_array_push(conf->customAttributes);

        if (!attribute) {
          return (char*)NGX_CONF_ERROR;
        }

        *attribute = kv.second;
      } else {
        CompiledScriptAttribute* attributes =
          (CompiledScriptAttribute*)conf->customAttributes->elts;
        attributes[index] = kv.second;
      }

      index++;
    }
  }

  return NGX_CONF_OK;
}

static ngx_int_t CreateOtelNgxVariables(ngx_conf_t* conf) {
  for (ngx_http_variable_t* v = otel_ngx_variables; v->name.len; v++) {
    ngx_http_variable_t* var = ngx_http_add_variable(conf, &v->name, v->flags);

    if (var == nullptr) {
      return NGX_ERROR;
    }

    var->get_handler = v->get_handler;
    var->set_handler = v->set_handler;
    var->data = v->data;
    v->index = var->index = ngx_http_get_variable_index(conf, &v->name);
  }

  return NGX_OK;
}

static ngx_http_module_t otel_ngx_http_module = {
  CreateOtelNgxVariables, /* preconfiguration */
  InitModule,             /* postconfiguration */
  CreateOtelMainConf,     /* create main conf */
  nullptr,                /* init main conf */
  nullptr,                /* create server conf */
  nullptr,                /* merge server conf */
  CreateOtelLocConf,      /* create loc conf */
  MergeLocConf,           /* merge loc conf */
};

bool CompileCommandScript(ngx_conf_t* ngxConf, ngx_command_t*, NgxCompiledScript* script) {
  ngx_str_t* value = (ngx_str_t*)ngxConf->args->elts;
  ngx_str_t* pattern = &value[1];

  return CompileScript(ngxConf, *pattern, script);
}

char* OtelNgxSetOperationNameVar(ngx_conf_t* ngxConf, ngx_command_t* cmd, void* conf) {
  auto locationConf = (OtelNgxLocationConf*)conf;
  if (CompileCommandScript(ngxConf, cmd, &locationConf->operationNameScript)) {
    return NGX_CONF_OK;
  }

  return (char*)NGX_CONF_ERROR;
}

struct HeaderPropagation {
  nostd::string_view directive;
  nostd::string_view parameter;
  nostd::string_view variable;
};

std::vector<HeaderPropagation> B3PropagationVars() {
  return {
    {"proxy_set_header", "b3", "$opentelemetry_context_b3"},
    {"fastcgi_param", "HTTP_B3", "$opentelemetry_context_b3"},
  };
}

std::vector<HeaderPropagation> OtelPropagationVars() {
  return {
    {"proxy_set_header", "traceparent", "$opentelemetry_context_traceparent"},
    {"proxy_set_header", "tracestate", "$opentelemetry_context_tracestate"},
    {"fastcgi_param", "HTTP_TRACEPARENT", "$opentelemetry_context_traceparent"},
    {"fastcgi_param", "HTTP_TRACESTATE", "$opentelemetry_context_tracestate"},
  };
}

char* OtelNgxSetPropagation(ngx_conf_t* conf, ngx_command_t*, void* locConf) {
  uint32_t numArgs = conf->args->nelts;

  auto locationConf = (OtelNgxLocationConf*)locConf;

  if (numArgs == 2) {
    ngx_str_t* args = (ngx_str_t*)conf->args->elts;
    nostd::string_view propagationType = FromNgxString(args[1]);

    if (propagationType == "b3") {
      locationConf->propagationType = TracePropagationB3;
    } else if (propagationType == "w3c") {
      locationConf->propagationType = TracePropagationW3C;
    } else {
      ngx_log_error(NGX_LOG_ERR, conf->log, 0, "Unsupported propagation type");
      return (char*)NGX_CONF_ERROR;
    }
  } else {
    locationConf->propagationType = TracePropagationW3C;
  }

  std::vector<HeaderPropagation> propagationVars;
  if (locationConf->propagationType == TracePropagationB3) {
    propagationVars = B3PropagationVars();
  } else {
    propagationVars = OtelPropagationVars();
  }

  ngx_array_t* oldArgs = conf->args;

  for (const HeaderPropagation& propagation : propagationVars) {
    ngx_str_t args[] = {
      ToNgxString(propagation.directive),
      ToNgxString(propagation.parameter),
      ToNgxString(propagation.variable),
    };

    ngx_array_t argsArray;
    ngx_memzero(&argsArray, sizeof(argsArray));

    argsArray.elts = &args;
    argsArray.nelts = 3;
    conf->args = &argsArray;

    if (OtelNgxConfHandler(conf, 0) != NGX_OK) {
      conf->args = oldArgs;
      return (char*)NGX_CONF_ERROR;
    }
  }

  conf->args = oldArgs;

  return NGX_CONF_OK;
}

char* OtelNgxSetConfig(ngx_conf_t* conf, ngx_command_t*, void*) {
  OtelMainConf* mainConf = (OtelMainConf*)ngx_http_conf_get_module_main_conf(conf, otel_ngx_module);

  ngx_str_t* values = (ngx_str_t*)conf->args->elts;
  ngx_str_t* path = &values[1];

  if (!OtelAgentConfigLoad(
        std::string((const char*)path->data, path->len), conf->log, &mainConf->agentConfig)) {
    return (char*)NGX_CONF_ERROR;
  }

  return NGX_CONF_OK;
}

static char* OtelNgxSetCustomAttribute(ngx_conf_t* conf, ngx_command_t*, void* userConf) {
  OtelNgxLocationConf* locConf = (OtelNgxLocationConf*)userConf;

  if (!locConf->customAttributes) {
    locConf->customAttributes = ngx_array_create(conf->pool, 1, sizeof(CompiledScriptAttribute));

    if (!locConf->customAttributes) {
      return (char*)NGX_CONF_ERROR;
    }
  }

  CompiledScriptAttribute* compiledAttribute =
    (CompiledScriptAttribute*)ngx_array_push(locConf->customAttributes);

  if (!compiledAttribute) {
    return (char*)NGX_CONF_ERROR;
  }

  ngx_str_t* args = (ngx_str_t*)conf->args->elts;

  ScriptAttributeDeclaration attrib{args[1], args[2]};
  if (!CompileScriptAttribute(conf, attrib, compiledAttribute)) {
    return (char*)NGX_CONF_ERROR;
  }

  return NGX_CONF_OK;
}

#if (NGX_PCRE)
static ngx_regex_t* NgxCompileRegex(ngx_conf_t* conf, ngx_str_t pattern) {
  u_char err[NGX_MAX_CONF_ERRSTR];

  ngx_regex_compile_t rc;
  ngx_memzero(&rc, sizeof(ngx_regex_compile_t));

  rc.pool = conf->pool;
  rc.pattern = pattern;
  rc.options = NGX_REGEX_CASELESS;
  rc.err.data = err;
  rc.err.len = sizeof(err);

  if (ngx_regex_compile(&rc) != NGX_OK) {
    ngx_log_error(NGX_LOG_ERR, conf->log, 0, "illegal regex in %V: %V", (ngx_str_t*)conf->args->elts, &rc.err);
    return nullptr;
  }

  return rc.regex;
}

static char* OtelNgxSetSensitiveHeaderNames(ngx_conf_t* conf, ngx_command_t*, void* userConf) {
  OtelNgxLocationConf* locConf = (OtelNgxLocationConf*)userConf;
  ngx_str_t* args = (ngx_str_t*)conf->args->elts;

  locConf->sensitiveHeaderNames = NgxCompileRegex(conf, args[1]);
  if (!locConf->sensitiveHeaderNames) {
    return (char*)NGX_CONF_ERROR;
  }

  return NGX_CONF_OK;
}

static char* OtelNgxSetSensitiveHeaderValues(ngx_conf_t* conf, ngx_command_t*, void* userConf) {
  OtelNgxLocationConf* locConf = (OtelNgxLocationConf*)userConf;
  ngx_str_t* args = (ngx_str_t*)conf->args->elts;

  locConf->sensitiveHeaderValues = NgxCompileRegex(conf, args[1]);
  if (!locConf->sensitiveHeaderValues) {
    return (char*)NGX_CONF_ERROR;
  }

  return NGX_CONF_OK;
}

static char* OtelNgxSetIgnorePaths(ngx_conf_t* conf, ngx_command_t*, void* userConf) {
  OtelNgxLocationConf* locConf = (OtelNgxLocationConf*)userConf;
  ngx_str_t* args = (ngx_str_t*)conf->args->elts;

  locConf->ignore_paths = NgxCompileRegex(conf, args[1]);
  if (!locConf->ignore_paths) {
    return (char*)NGX_CONF_ERROR;
  }

  return NGX_CONF_OK;
}
#endif

static ngx_command_t kOtelNgxCommands[] = {
  {
    ngx_string("opentelemetry_propagate"),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS | NGX_CONF_TAKE1,
    OtelNgxSetPropagation,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    nullptr,
  },
  {
    ngx_string("opentelemetry_operation_name"),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    OtelNgxSetOperationNameVar,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    nullptr,
  },
  {
    ngx_string("opentelemetry_config"),
    NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
    OtelNgxSetConfig,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    nullptr,
  },
  {
    ngx_string("opentelemetry_attribute"),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE2,
    OtelNgxSetCustomAttribute,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    nullptr,
  },
  {
    ngx_string("opentelemetry"),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(OtelNgxLocationConf, enabled),
    nullptr,
  },
  {
    ngx_string("opentelemetry_trust_incoming_spans"),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(OtelNgxLocationConf, trustIncomingSpans),
    nullptr,
  },
  {
    ngx_string("opentelemetry_capture_headers"),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(OtelNgxLocationConf, captureHeaders),
    nullptr,
  },
#if (NGX_PCRE)
  {
    ngx_string("opentelemetry_sensitive_header_names"),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    OtelNgxSetSensitiveHeaderNames,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    nullptr,
  },
  {
    ngx_string("opentelemetry_sensitive_header_values"),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    OtelNgxSetSensitiveHeaderValues,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    nullptr,
  },
  {
    ngx_string("opentelemetry_ignore_paths"),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    OtelNgxSetIgnorePaths,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    nullptr,
  },
#endif
  ngx_null_command,
};

static std::unique_ptr<sdktrace::SpanExporter> CreateExporter(const OtelNgxAgentConfig* conf) {
  std::unique_ptr<sdktrace::SpanExporter> exporter;

  switch (conf->exporter.type) {
    case OtelExporterOTLP: {
      std::string endpoint = conf->exporter.endpoint;
      otlp::OtlpGrpcExporterOptions opts{endpoint};
      opts.use_ssl_credentials = conf->exporter.use_ssl_credentials;
      opts.ssl_credentials_cacert_path = conf->exporter.ssl_credentials_cacert_path;
      exporter.reset(new otlp::OtlpGrpcExporter(opts));
      break;
    }
    default:
      break;
  }

  return exporter;
}

static std::unique_ptr<sdktrace::SpanProcessor>
CreateProcessor(const OtelNgxAgentConfig* conf, std::unique_ptr<sdktrace::SpanExporter> exporter) {
  if (conf->processor.type == OtelProcessorBatch) {
    sdktrace::BatchSpanProcessorOptions opts;
    opts.max_queue_size = conf->processor.batch.maxQueueSize;
    opts.schedule_delay_millis =
      std::chrono::milliseconds(conf->processor.batch.scheduleDelayMillis);
    opts.max_export_batch_size = conf->processor.batch.maxExportBatchSize;

    return std::unique_ptr<sdktrace::SpanProcessor>(
      new sdktrace::BatchSpanProcessor(std::move(exporter), opts));
  }

  return std::unique_ptr<sdktrace::SpanProcessor>(
    new sdktrace::SimpleSpanProcessor(std::move(exporter)));
}

static std::unique_ptr<sdktrace::Sampler> CreateSampler(const OtelNgxAgentConfig* conf) {
  if (conf->sampler.parentBased) {
    std::shared_ptr<sdktrace::Sampler> sampler;

    switch (conf->sampler.type) {
      case OtelSamplerAlwaysOn: {
        sampler = std::make_shared<sdktrace::AlwaysOnSampler>();
        break;
      }
      case OtelSamplerAlwaysOff: {
        sampler = std::make_shared<sdktrace::AlwaysOffSampler>();
        break;
      }
      case OtelSamplerTraceIdRatioBased: {
        sampler = std::make_shared<sdktrace::TraceIdRatioBasedSampler>(conf->sampler.ratio);
        break;
      }
      default:
        break;
    }

    return std::unique_ptr<sdktrace::ParentBasedSampler>(new sdktrace::ParentBasedSampler(sampler));
  }

  std::unique_ptr<sdktrace::Sampler> sampler;

  switch (conf->sampler.type) {
    case OtelSamplerAlwaysOn: {
      sampler.reset(new sdktrace::AlwaysOnSampler());
      break;
    }
    case OtelSamplerAlwaysOff: {
      sampler.reset(new sdktrace::AlwaysOffSampler());
      break;
    }
    case OtelSamplerTraceIdRatioBased: {
      sampler.reset(new sdktrace::TraceIdRatioBasedSampler(conf->sampler.ratio));
      break;
    }
    default:
      break;
  }
  return sampler;
}

static ngx_int_t OtelNgxStart(ngx_cycle_t* cycle) {
  OtelMainConf* otelMainConf =
    (OtelMainConf*)ngx_http_cycle_get_module_main_conf(cycle, otel_ngx_module);

  OtelNgxAgentConfig* agentConf = &otelMainConf->agentConfig;

  auto exporter = CreateExporter(agentConf);

  if (!exporter) {
    ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "Unable to create span exporter - invalid type");
    return NGX_ERROR;
  }

  auto sampler = CreateSampler(agentConf);

  if (!sampler) {
    ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "Unable to create sampler - invalid type");
    return NGX_ERROR;
  }

  auto processor = CreateProcessor(agentConf, std::move(exporter));
  auto provider =
    nostd::shared_ptr<opentelemetry::trace::TracerProvider>(new sdktrace::TracerProvider(
      std::move(processor),
      opentelemetry::sdk::resource::Resource::Create({{"service.name", agentConf->service.name}}),
      std::move(sampler)));

  opentelemetry::trace::Provider::SetTracerProvider(std::move(provider));

  return NGX_OK;
}

ngx_module_t otel_ngx_module = {
  NGX_MODULE_V1,
  &otel_ngx_http_module,
  kOtelNgxCommands,
  NGX_HTTP_MODULE,
  nullptr,      /* init master */
  nullptr,      /* init module - prior to forking from master process */
  OtelNgxStart, /* init process - worker process fork */
  nullptr,      /* init thread */
  nullptr,      /* exit thread */
  nullptr,      /* exit process - worker process exit */
  nullptr,      /* exit master */
  NGX_MODULE_V1_PADDING,
};
