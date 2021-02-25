#pragma once

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

struct NgxCompiledScript {
  ngx_str_t pattern = ngx_null_string;
  ngx_array_t* lengths = nullptr;
  ngx_array_t* values = nullptr;

  bool Run(ngx_http_request_t* req, ngx_str_t* result) {
    if (!lengths) {
      *result = pattern;
      return true;
    }

    if (!ngx_http_script_run(req, result, lengths->elts, 0, values->elts)) {
      *result = ngx_null_string;
      return false;
    }

    return true;
  }
};

bool CompileScript(ngx_conf_t* conf, ngx_str_t pattern, NgxCompiledScript* script);

