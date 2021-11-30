#pragma once

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include "nginx_utils.h"

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

  bool IsEmpty() {
    return pattern.len == 0;
  }
};

enum ScriptAttributeType {
  ScriptAttributeInt,
  ScriptAttributeString,
};

struct ScriptAttributeDeclaration {
  ScriptAttributeDeclaration(
    opentelemetry::nostd::string_view attribute, opentelemetry::nostd::string_view script, ScriptAttributeType type = ScriptAttributeString)
    : attribute(ToNgxString(attribute)), script(ToNgxString(script)), type(type) {}

  ScriptAttributeDeclaration(ngx_str_t attribute, ngx_str_t script)
    : attribute(attribute), script(script), type(ScriptAttributeString) {}

  ngx_str_t attribute;
  ngx_str_t script;
  ScriptAttributeType type;
};

struct CompiledScriptAttribute {
  NgxCompiledScript key;
  NgxCompiledScript value;
  ScriptAttributeType type;
};

bool CompileScript(ngx_conf_t* conf, ngx_str_t pattern, NgxCompiledScript* script);
bool CompileScriptAttribute(
  ngx_conf_t* conf, ScriptAttributeDeclaration declaration,
  CompiledScriptAttribute* compiledAttribute);
