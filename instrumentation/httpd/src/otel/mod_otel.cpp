/*
 * Copyright The OpenTelemetry Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "opentelemetry.h"

#include "opentelemetry/context/context.h"
#include "opentelemetry/trace/propagation/b3_propagator.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"

#include "ap_config.h"
#include "httpd.h"
// httpd.h has to be before those two and after opentelemetry because it #define's OK 0
#include "http_config.h"
#include "http_protocol.h"
#include "mod_proxy.h"

namespace
{

const std::string kSpanNamePrefix = "HTTP ";

using namespace httpd_otel;

const char kOpenTelemetryKeyNote[] = "OTEL";
const char kOpenTelemetryKeyOutboundNote[] = "OTEL_PROXY";

const char kEnvVarSpanId[] = "OTEL_SPANID";
const char kEnvVarTraceId[] = "OTEL_TRACEID";
const char kEnvVarTraceFlags[] = "OTEL_TRACEFLAGS";
const char kEnvVarTraceState[] = "OTEL_TRACESTATE";

class HttpdCarrier : public opentelemetry::context::propagation::TextMapCarrier
{
public:
  apr_table_t& hdrs;
  HttpdCarrier(apr_table_t& headers):hdrs(headers){}
  virtual opentelemetry::v0::nostd::string_view Get(opentelemetry::v0::nostd::string_view key) const noexcept override
  {
    auto fnd = apr_table_get(&hdrs, std::string(key).c_str());
    return fnd ? fnd : "";
  }
  virtual void Set(opentelemetry::v0::nostd::string_view key, opentelemetry::v0::nostd::string_view value) noexcept override
  {
    apr_table_set(&hdrs, std::string(key).c_str(),
              std::string(value).c_str());
  }
};

// propagators
opentelemetry::trace::propagation::HttpTraceContext PropagatorTraceContext;
opentelemetry::trace::propagation::B3Propagator PropagatorB3SingleHeader;
opentelemetry::trace::propagation::B3PropagatorMultiHeader PropagatorB3MultiHeader;

// from:
// https://github.com/open-telemetry/opentelemetry-specification/blob/master/specification/trace/semantic_conventions/http.md#http-server-semantic-conventions
HttpdStartSpanAttributes GetAttrsFromRequest(request_rec *r)
{
   HttpdStartSpanAttributes res;
   res.server_name = r->server->server_hostname;
   if (r->method)
   {
    res.method = r->method;
   }
   res.scheme = ap_run_http_scheme(r);
   if (r->hostname)
   {
     res.host = r->hostname;
   }
   if (r->unparsed_uri)
   {
     res.target = r->unparsed_uri;
   }
   switch (r->proto_num)
   {  // TODO: consider using ap_get_protocol for other flavors
      case 1000:
        res.flavor = "1.0";
        break;
      case 1001:
        res.flavor = "1.1";
        break;
  }
  res.client_ip = r->useragent_ip;
  res.net_ip = r->connection->client_ip;
  return res;
}

// main function
// TODO: fix this for scenarios where apache configuration is just updated (apachectl -k graceful)
static void opentel_child_created(apr_pool_t*, server_rec*)
{
  initTracer();
}

// populates environment variables which can be used by other parts of httpd
void addEnvVars(apr_table_t *envTable, opentelemetry::trace::SpanContext ctx)
{
  union {
    char bfr[33] = {0};
    char spanId[16];
    char traceId[32];
  } ;

  if (ctx.trace_flags().IsSampled())
  {
    apr_table_set(envTable, kEnvVarTraceFlags, "1");
  }
  // to keep bfr null-terminated we start from shorter (spanId)
  ctx.span_id().ToLowerBase16(spanId);
  apr_table_set(envTable, kEnvVarSpanId, bfr);
  ctx.trace_id().ToLowerBase16(traceId);
  apr_table_set(envTable, kEnvVarTraceId, bfr);

  apr_table_set(envTable, kEnvVarTraceState, ctx.trace_state()->ToHeader().c_str());
}

// Starting span as early as possible (quick handler):
// http://www.fmc-modeling.org/category/projects/apache/amp/3_3Extending_Apache.html#fig:_Apache:_request-processing_+_Module_callbacks_PN
static int opentel_handler(request_rec *r, int /* lookup_uri */ )
{
  // track main request
  // TODO: find a scenario where it makes a difference (perhaps something with mod_rewrite?)
  request_rec *req = r->main ? r->main : r;

  ExtraRequestData *req_data;
  apr_pool_userdata_get((void **)&req_data, kOpenTelemetryKeyNote, req->pool);

  if (req_data)
  {  // we already have span started
    return DECLINED;
  }

  // start new span
  // re-use exisiting memory pool provided by apache
  auto ExtraRequestDataBuffer = apr_palloc(req->pool, sizeof(ExtraRequestData));
  // and use placement new
  req_data = new (ExtraRequestDataBuffer) ExtraRequestData;

  apr_pool_userdata_setn(ExtraRequestDataBuffer, kOpenTelemetryKeyNote,
                         (apr_status_t(*)(void *))ExtraRequestData::Destruct, req->pool);

  if (!config.ignore_inbound && config.propagation != OtelPropagation::NONE)
  {
    HttpdCarrier car(*req->headers_in);
    opentelemetry::v0::context::Context ctx_new,
        ctx_cur = opentelemetry::context::RuntimeContext::GetCurrent();
    switch (config.propagation)
    {
      default:
        ctx_new = PropagatorTraceContext.Extract(car, ctx_cur);
        break;
      case OtelPropagation::B3_SINGLE_HEADER:
      case OtelPropagation::B3_MULTI_HEADER:
        ctx_new = PropagatorB3SingleHeader.Extract(car, ctx_cur);
    }
    req_data->token = opentelemetry::context::RuntimeContext::Attach(ctx_new);
  }

  opentelemetry::trace::StartSpanOptions startOpts;
  startOpts.kind = opentelemetry::trace::SpanKind::kServer;
  auto span = get_tracer()->StartSpan(kSpanNamePrefix + req->method, startOpts);

  req_data->span  = span;
  req_data->StartSpan(GetAttrsFromRequest(req));

  addEnvVars(req->subprocess_env, span->GetContext());

  return DECLINED;
}

static int opentel_log_transaction(request_rec *r)
{
  request_rec *req = r->main ? r->main : r;

  ExtraRequestData *req_data;
  apr_pool_userdata_get((void **)&req_data, kOpenTelemetryKeyNote, req->pool);

  if (!req_data)
  {
    return DECLINED;
  }
  // finish span
  req_data->EndSpan({
    req->status,
    req->bytes_sent
  });

  return DECLINED;
}

/////////////////////////////////////////////////
// Outbound span created when mod_proxy is used
/////////////////////////////////////////////////

static int proxy_fixup_handler(request_rec *r)
{ // adding outbound headers and setting span attributes
  request_rec *req = r->main ? r->main : r;

  ExtraRequestData *req_data;
  apr_pool_userdata_get((void **)&req_data, kOpenTelemetryKeyNote, req->pool);

  if (!req_data)
  {  // main (parent) span not started?
    return DECLINED;
  }

  ExtraRequestData *req_data_out;
  apr_pool_userdata_get((void **)&req_data_out, kOpenTelemetryKeyOutboundNote, req->pool);

  if (req_data_out)
  {  // we already have span started
    return DECLINED;
  }

  // start new span
  // re-use exisiting memory pool provided by apache
  auto ExtraRequestDataBuffer = apr_palloc(r->pool, sizeof(ExtraRequestData));
  // and use placement new
  req_data_out = new (ExtraRequestDataBuffer) ExtraRequestData;

  apr_pool_userdata_setn(ExtraRequestDataBuffer, kOpenTelemetryKeyOutboundNote,
                         (apr_status_t(*)(void *))ExtraRequestData::Destruct, req->pool);

  opentelemetry::trace::StartSpanOptions startOpts;
  startOpts.kind = opentelemetry::trace::SpanKind::kClient;
  startOpts.parent = req_data->span->GetContext();
  auto span = get_tracer()->StartSpan(kSpanNamePrefix + req->method, startOpts);

  req_data_out->span  = span;
  HttpdStartSpanAttributes startAttrs;
  startAttrs.server_name = r->server->server_hostname;
  startAttrs.method = req->method;
  startAttrs.url = req->filename;
  if (startAttrs.url.substr(0,6) == "proxy:")
  {
    startAttrs.url = startAttrs.url.substr(6);
  }

  req_data_out->StartSpan(startAttrs);

  auto scope = get_tracer()->WithActiveSpan(span);

  // mod_proxy simply copies request headers from client therefore inject is into headers_in
  // instead of headers_out
  HttpdCarrier car(*req->headers_in);
  switch (config.propagation)
  {
    case OtelPropagation::TRACE_CONTEXT:
      PropagatorTraceContext.Inject(car, opentelemetry::context::RuntimeContext::GetCurrent());
      break;
    case OtelPropagation::B3_SINGLE_HEADER:
      PropagatorB3SingleHeader.Inject(car, opentelemetry::context::RuntimeContext::GetCurrent());
      break;
    case OtelPropagation::B3_MULTI_HEADER:
      PropagatorB3MultiHeader.Inject(car, opentelemetry::context::RuntimeContext::GetCurrent());
      break;
    default:  // suppress warning
      break;
  }

  return DECLINED;
}

static int proxy_end_handler(int *status, request_rec *r)
{
  request_rec *req = r->main ? r->main : r;

  ExtraRequestData *req_data_out;
  apr_pool_userdata_get((void **)&req_data_out, kOpenTelemetryKeyOutboundNote, req->pool);

  if (!req_data_out)
  {
    return DECLINED;
  }

  int st_code = (status && *status) ? *status:r->status;
  const char *proxy_error = apr_table_get(r->notes, "error-notes");
  if (proxy_error)
  {
    req_data_out->span->SetStatus(opentelemetry::trace::StatusCode::kError, proxy_error);
  }

  // finish span
  req_data_out->EndSpan({
    st_code,
    req->bytes_sent
  });
  req_data_out->Destruct(req_data_out);

  return DECLINED;
}

/////////////////////////////////////////////////
// PARSING CONFIGURATION OPTIONS
/////////////////////////////////////////////////

const char *otel_set_exporter(cmd_parms* /* cmd */, void */* cfg */, const char *arg)
{
  if (!strcasecmp(arg, "file"))
    config.type = OtelExporterType::OSTREAM;
  else if (!strcasecmp(arg, "otlp"))
    config.type = OtelExporterType::OTLP;
  else
    return "Unknown exporter type - can be file or otlp";

  return NULL;
}

const char *otel_set_propagator(cmd_parms* /* cmd */, void* /* cfg */, const char *arg)
{
  if (!strcasecmp(arg, "trace-context"))
    config.propagation = OtelPropagation::TRACE_CONTEXT;
  else if (!strcasecmp(arg, "b3"))
    config.propagation = OtelPropagation::B3_SINGLE_HEADER;
  else if (!strcasecmp(arg, "b3-multiheader"))
    config.propagation = OtelPropagation::B3_MULTI_HEADER;
  else
    return "Unknown propagator type - can be trace-context or b3 or b3-multiheader";

  return NULL;
}

const char *otel_set_ignoreInbound(cmd_parms* /* cmd */, void* /* cfg */, int flag)
{
  config.ignore_inbound = flag;
  return NULL;
}

const char *otel_set_path(cmd_parms* /* cmd */, void* /* cfg */, const char *arg)
{
  config.fname = arg;
  return NULL;
}

const char *otel_set_endpoint(cmd_parms* /* cmd */, void* /* cfg */, const char *arg)
{
  config.endpoint = arg;
  return NULL;
}

const char *otel_set_attribute(cmd_parms* /* cmd */, void* /* cfg */, const char *attrName, const char *attrValue)
{
  config.attributes[attrName] = attrValue;
  return NULL;
}

const char *otel_set_resource(cmd_parms* /* cmd */, void* /* cfg */, const char *attrName, const char *attrValue)
{
  config.resources[attrName] = attrValue;
  return NULL;
}

const char *otel_cfg_batch(cmd_parms* /* cmd */,
                           void* /* cfg */,
                           const char *max_queue_size,
                           const char *schedule_delay_millis,
                           const char *max_export_batch_size)
{
  config.batch_opts.max_queue_size        = atoi(max_queue_size);
  config.batch_opts.schedule_delay_millis = std::chrono::milliseconds(atoi(schedule_delay_millis));
  config.batch_opts.max_export_batch_size = atoi(max_export_batch_size);
  return NULL;
}

}  // namespace

extern "C" {

static void opentel_register_hooks(apr_pool_t* /* p */)
{
  ap_hook_child_init(opentel_child_created, NULL, NULL, APR_HOOK_MIDDLE);
  ap_hook_quick_handler(opentel_handler, NULL, NULL, APR_HOOK_FIRST);
  ap_hook_log_transaction(opentel_log_transaction, NULL, NULL, APR_HOOK_LAST);
  // for outbound transactions (proxy hooks)
  // I've tried scheme_handler, canon_handler but they weren't working
  // pre_request is only for balancer:// schema
  APR_OPTIONAL_HOOK(proxy, fixups, proxy_fixup_handler, NULL, NULL, APR_HOOK_REALLY_FIRST);
  APR_OPTIONAL_HOOK(proxy, request_status, proxy_end_handler, NULL, NULL,
                     APR_HOOK_MIDDLE);
}

static const command_rec opentel_directives[] = {
    AP_INIT_TAKE1("OpenTelemetryExporter",
                  reinterpret_cast<cmd_func>(otel_set_exporter),
                  NULL,
                  RSRC_CONF,
                  "Set specific exporter type"),
    AP_INIT_TAKE1("OpenTelemetryPath", reinterpret_cast<cmd_func>(otel_set_path), NULL, RSRC_CONF, "Set path for exporter"),
    AP_INIT_TAKE1("OpenTelemetryEndpoint",
                  reinterpret_cast<cmd_func>(otel_set_endpoint),
                  NULL,
                  RSRC_CONF,
                  "Set endpoint for exporter"),
    AP_INIT_TAKE2("OpenTelemetrySetAttribute",
                  reinterpret_cast<cmd_func>(otel_set_attribute),
                  NULL,
                  RSRC_CONF,
                  "Set additional attribute for each span"),
    AP_INIT_TAKE2("OpenTelemetrySetResource",
                  reinterpret_cast<cmd_func>(otel_set_resource),
                  NULL,
                  RSRC_CONF,
                  "Set resource"),
    AP_INIT_TAKE3("OpenTelemetryBatch",
                  reinterpret_cast<cmd_func>(otel_cfg_batch),
                  NULL,
                  RSRC_CONF,
                  "Configure batch processing"),
    AP_INIT_FLAG("OpenTelemetryIgnoreInbound",
                  reinterpret_cast<cmd_func>(otel_set_ignoreInbound),
                  NULL,
                  RSRC_CONF,
                  "Enable or disable context propagation from incoming requests."),
    AP_INIT_TAKE1("OpenTelemetryPropagators",
                  reinterpret_cast<cmd_func>(otel_set_propagator),
                  NULL,
                  RSRC_CONF,
                  "Configure propagators"),
    {NULL}};

/* the main config structure */
AP_DECLARE_MODULE(otel) = {
    STANDARD20_MODULE_STUFF,
    NULL,                  /* create per-dir    config structures */
    NULL,                  /* merge  per-dir    config structures */
    NULL,                  /* create per-server config structures */
    NULL,                  /* merge  per-server config structures */
    opentel_directives,    /* table of config file commands       */
    opentel_register_hooks /* register hooks                      */
};
}  // extern "C"
