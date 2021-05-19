#include <opentelemetry/nostd/mpark/variant.h>
#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/trace/span.h>
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
#include <opentelemetry/exporters/otlp/otlp_exporter.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/trace/provider.h>

namespace trace = opentelemetry::trace;
namespace nostd = opentelemetry::nostd;
namespace sdktrace = opentelemetry::sdk::trace;
namespace otlp = opentelemetry::exporter::otlp;

constexpr char kOtelCtxVarPrefix[] = "otel_ctxvar_";

const ScriptAttributeDeclaration kDefaultScriptAttributes[] = {
  {"http.scheme", "$scheme"},
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

static ngx_int_t OtelGetContextVar(ngx_http_request_t*, ngx_http_variable_value_t*, uintptr_t) {
  // Filled out on context creation.
  return NGX_OK;
}

static ngx_int_t
OtelGetTraceContextVar(ngx_http_request_t* req, ngx_http_variable_value_t* v, uintptr_t data);

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
  ngx_http_null_variable,
};

TraceContext* GetTraceContext(ngx_http_request_t* req) {
  ngx_http_variable_value_t* val = ngx_http_get_indexed_variable(req, otel_ngx_variables[0].index);

  if (val == nullptr || val->not_found) {
    ngx_log_error(NGX_LOG_ERR, req->connection->log, 0, "TraceContext not found");
    return nullptr;
  }

  return (TraceContext*)val->data;
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
  TraceContext* traceContext = GetTraceContext(req);

  if (traceContext == nullptr || !traceContext->request_span) {
    ngx_log_error(
      NGX_LOG_ERR, req->connection->log, 0,
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

void TraceContextCleanup(void* data) {
  TraceContext* context = (TraceContext*)data;
  context->~TraceContext();
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

static bool IsOtelEnabled(ngx_http_request_t* req) {
  OtelNgxLocationConf* locConf = GetOtelLocationConf(req);
  return locConf->enabled;
}

ngx_int_t StartNgxSpan(ngx_http_request_t* req) {
  if (!IsOtelEnabled(req)) {
    return NGX_DECLINED;
  }

  ngx_http_variable_value_t* val = ngx_http_get_indexed_variable(req, otel_ngx_variables[0].index);

  if (!val) {
    ngx_log_error(NGX_LOG_ERR, req->connection->log, 0, "Unable to find OpenTelemetry context");
    return NGX_DECLINED;
  }

  ngx_pool_cleanup_t* cleanup = ngx_pool_cleanup_add(req->pool, sizeof(TraceContext));
  TraceContext* context = (TraceContext*)cleanup->data;
  new (context) TraceContext(req);
  cleanup->handler = TraceContextCleanup;

  val->data = (unsigned char*)cleanup->data;
  val->len = sizeof(TraceContext);

  OtelCarrier carrier{req, context};
  auto incomingContext = ExtractContext(&carrier);

  trace::StartSpanOptions startOpts;
  startOpts.kind = trace::SpanKind::kServer;
  startOpts.parent = GetCurrentSpan(incomingContext);

  context->request_span = GetTracer()->StartSpan(
    GetOperationName(req),
    {
      {"http.method", FromNgxString(req->method_name)},
      {"http.flavor", NgxHttpFlavor(req)},
      {"http.target", FromNgxString(req->unparsed_uri)},
      {"http.host", FromNgxString(req->headers_in.host->value)},
    },
    startOpts);

  nostd::string_view serverName = GetNgxServerName(req);
  if (!serverName.empty()) {
    context->request_span->SetAttribute("http.server_name", serverName);
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
      span->SetAttribute(FromNgxString(key), FromNgxString(value));
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

  AddScriptAttributes(span.get(), GetOtelMainConf(req)->scriptAttributes, req);
  AddScriptAttributes(span.get(), GetOtelLocationConf(req)->customAttributes, req);

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
    {"proxy_set_header", "b3", "$otel_ctxvar_b3"},
    {"fastcgi_param", "HTTP_B3", "$otel_ctxvar_b3"},
  };
}

std::vector<HeaderPropagation> OtelPropagationVars() {
  return {
    {"proxy_set_header", "traceparent", "$otel_ctxvar_traceparent"},
    {"proxy_set_header", "tracestate", "$otel_ctxvar_tracestate"},
    {"fastcgi_param", "HTTP_TRACEPARENT", "$otel_ctxvar_traceparent"},
    {"fastcgi_param", "HTTP_TRACESTATE", "$otel_ctxvar_tracestate"},
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
  ngx_null_command,
};

static std::unique_ptr<sdktrace::SpanExporter> CreateExporter(const OtelNgxAgentConfig* conf) {
  std::unique_ptr<sdktrace::SpanExporter> exporter;

  switch (conf->exporter.type) {
    case OtelExporterOTLP: {
      std::string endpoint = conf->exporter.endpoint;
      otlp::OtlpExporterOptions opts{endpoint};
      exporter.reset(new otlp::OtlpExporter(opts));
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

static ngx_int_t OtelNgxStart(ngx_cycle_t* cycle) {
  OtelMainConf* otelMainConf =
    (OtelMainConf*)ngx_http_cycle_get_module_main_conf(cycle, otel_ngx_module);

  OtelNgxAgentConfig* agentConf = &otelMainConf->agentConfig;

  auto exporter = CreateExporter(agentConf);

  if (!exporter) {
    ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "Unable to create span exporter - invalid type");
    return NGX_ERROR;
  }

  auto processor = CreateProcessor(agentConf, std::move(exporter));
  auto provider = nostd::shared_ptr<opentelemetry::trace::TracerProvider>(new sdktrace::TracerProvider(
    std::move(processor),
    opentelemetry::sdk::resource::Resource::Create({{"service.name", agentConf->service.name}})));

  opentelemetry::trace::Provider::SetTracerProvider(provider);

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
