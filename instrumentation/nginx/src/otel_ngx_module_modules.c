#include <ngx_config.h>
#include <ngx_core.h>

extern ngx_module_t otel_ngx_module;

ngx_module_t* ngx_modules[] = {
  &otel_ngx_module,
  NULL,
};

char* ngx_module_names[] = {
  "otel_ngx_module",
  NULL,
};

char* ngx_module_order[] = {
  NULL,
};
