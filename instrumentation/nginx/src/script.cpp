#include "script.h"

bool CompileScript(ngx_conf_t* conf, ngx_str_t pattern, NgxCompiledScript* script) {
  script->pattern = pattern;
  script->lengths = nullptr;
  script->values = nullptr;

  ngx_uint_t numVariables = ngx_http_script_variables_count(&script->pattern);

  if (numVariables == 0) {
    return true;
  }

  ngx_http_script_compile_t compilation;
  ngx_memzero(&compilation, sizeof(compilation));
  compilation.cf = conf;
  compilation.source = &script->pattern;
  compilation.lengths = &script->lengths;
  compilation.values = &script->values;
  compilation.variables = numVariables;
  compilation.complete_lengths = 1;
  compilation.complete_values = 1;

  return ngx_http_script_compile(&compilation) == NGX_OK;
}

bool CompileScriptAttribute(
  ngx_conf_t* conf, ScriptAttributeDeclaration declaration,
  CompiledScriptAttribute* compiledAttribute) {
  compiledAttribute->type = declaration.type;

  if (!CompileScript(conf, declaration.attribute, &compiledAttribute->key)) {
    return false;
  }

  return CompileScript(conf, declaration.script, &compiledAttribute->value);
}
