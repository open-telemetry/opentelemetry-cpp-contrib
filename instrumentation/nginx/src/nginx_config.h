#pragma once

extern "C" {
#include <ngx_core.h>
}

ngx_int_t OtelNgxConfHandler(ngx_conf_t* conf, ngx_int_t last);
