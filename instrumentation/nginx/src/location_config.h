#pragma once

#include "trace_context.h"
#include "script.h"

extern "C" {
extern ngx_module_t otel_ngx_module;
}

struct OtelNgxLocationConf {
  ngx_flag_t enabled = NGX_CONF_UNSET;
  TracePropagationType propagationType = TracePropagationW3C;
  NgxCompiledScript operationNameScript;
};

inline OtelNgxLocationConf* GetOtelLocationConf(ngx_http_request_t* req) {
  return (OtelNgxLocationConf*)ngx_http_get_module_loc_conf(req, otel_ngx_module);
}
