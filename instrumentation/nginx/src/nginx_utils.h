#pragma once

#include <opentelemetry/nostd/string_view.h>

extern "C" {
#include <ngx_string.h>
}

inline opentelemetry::nostd::string_view FromNgxString(ngx_str_t str) {
  return {(const char*)str.data, str.len};
}
inline ngx_str_t ToNgxString(opentelemetry::nostd::string_view str) {
  return {str.size(), (u_char*)str.data()};
}
