#pragma once

#include "trace_context.h"
#include "script.h"

extern "C" {
extern ngx_module_t otel_ngx_module;
}

struct OtelNgxLocationConf {
  ngx_flag_t enabled = NGX_CONF_UNSET;
  ngx_flag_t trustIncomingSpans = NGX_CONF_UNSET;
  ngx_flag_t captureHeaders = NGX_CONF_UNSET;
#if (NGX_PCRE)
  ngx_regex_t *sensitiveHeaderNames = (ngx_regex_t*)NGX_CONF_UNSET_PTR;
  ngx_regex_t *sensitiveHeaderValues = (ngx_regex_t*)NGX_CONF_UNSET_PTR;
  ngx_regex_t *ignore_paths = (ngx_regex_t*)NGX_CONF_UNSET_PTR;
#endif
  TracePropagationType propagationType = TracePropagationUnset;
  NgxCompiledScript operationNameScript;
  ngx_array_t* customAttributes = nullptr;
};

inline OtelNgxLocationConf* GetOtelLocationConf(ngx_http_request_t* req) {
  return (OtelNgxLocationConf*)ngx_http_get_module_loc_conf(req, otel_ngx_module);
}
