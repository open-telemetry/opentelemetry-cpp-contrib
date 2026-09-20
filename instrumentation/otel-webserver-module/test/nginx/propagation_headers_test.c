/*
 * Copyright 2026, OpenTelemetry Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef unsigned long ngx_uint_t;
typedef long ngx_int_t;
typedef long ngx_flag_t;

#define NGX_OK 0
#define NGX_ERROR -1

typedef struct ngx_list_part_s ngx_list_part_t;
struct ngx_list_part_s
{
  void *elts;
  ngx_uint_t nelts;
  ngx_list_part_t *next;
};

typedef struct
{
  size_t len;
  unsigned char *data;
} ngx_str_t;

typedef struct
{
  ngx_str_t key;
  ngx_str_t value;
} ngx_table_elt_t;

typedef struct
{
  char *name;
  char *value;
} http_headers;

const char *httpHeaders[] = {"x-b3-traceid", "x-b3-spanid", "x-b3-sampled", "traceparent",
                             "tracestate"};
const size_t headers_len  = sizeof(httpHeaders) / sizeof(httpHeaders[0]);

#include "../../src/nginx/ngx_http_opentelemetry_module_internal.h"

static void init_headers(ngx_table_elt_t *headers, size_t count, const char *name)
{
  for (size_t i = 0; i < count; i++)
  {
    headers[i].key.data   = (unsigned char *)name;
    headers[i].key.len    = strlen(name);
    headers[i].value.data = (unsigned char *)"value";
    headers[i].value.len  = 5;
  }
}

static void test_repeated_headers(size_t count)
{
  ngx_table_elt_t first_headers[20];
  ngx_table_elt_t remaining_headers[380];
  ngx_list_part_t second = {remaining_headers, count - 20, NULL};
  ngx_list_part_t first  = {first_headers, 20, &second};
  size_t capacity        = 0;
  size_t allocation_size = 0;

  assert(count >= 20 && count <= 400);
  init_headers(first_headers, 20, "traceparent");
  init_headers(remaining_headers, count - 20, "traceparent");

  assert(ngx_http_otel_header_list_capacity(&first, &capacity, &allocation_size) == NGX_OK);
  assert(capacity == count);
  assert(allocation_size == count * sizeof(http_headers));

  http_headers propagation_headers[400] = {{0}};
  size_t populated = 0;
  assert(ngx_http_otel_fill_propagation_headers(
             &first, propagation_headers, capacity, 1, &populated) == NGX_OK);
  assert(populated == count);
  assert(populated <= capacity);
}

static void test_no_propagation_headers(void)
{
  ngx_table_elt_t headers[25];
  ngx_list_part_t part                 = {headers, 25, NULL};
  http_headers propagation_headers[25] = {{0}};

  init_headers(headers, 25, "accept");
  size_t populated = 1;
  assert(ngx_http_otel_fill_propagation_headers(
             &part, propagation_headers, 25, 1, &populated) == NGX_OK);
  assert(populated == 0);
}

static void test_trust_incoming_spans_disabled(void)
{
  ngx_table_elt_t headers[25];
  ngx_list_part_t part                 = {headers, 25, NULL};
  http_headers propagation_headers[25] = {{0}};

  init_headers(headers, 25, "traceparent");
  size_t populated = 1;
  assert(ngx_http_otel_fill_propagation_headers(
             &part, propagation_headers, 25, 0, &populated) == NGX_OK);
  assert(populated == 0);
}

static void test_allocation_failure_invariant(void)
{
  ngx_table_elt_t headers[25];
  ngx_list_part_t part = {headers, 25, NULL};

  init_headers(headers, 25, "traceparent");
  size_t populated = 1;
  assert(ngx_http_otel_fill_propagation_headers(&part, NULL, 25, 1, &populated) == NGX_OK);
  assert(populated == 0);
}

static void test_capacity_guard(void)
{
  ngx_table_elt_t headers[25];
  ngx_list_part_t part = {headers, 25, NULL};
  http_headers guarded[11];

  init_headers(headers, 25, "traceparent");
  memset(guarded, 0, sizeof(guarded));
  guarded[10].name = (char *)"canary";

  size_t populated = 1;
  assert(ngx_http_otel_fill_propagation_headers(
             &part, guarded, 10, 1, &populated) == NGX_ERROR);
  assert(populated == 0);
  assert(strcmp(guarded[10].name, "canary") == 0);
}

static void test_zero_and_overflow_capacity(void)
{
  ngx_list_part_t zero     = {NULL, 0, NULL};
  ngx_list_part_t overflow = {NULL, (ngx_uint_t)INT_MAX + 1U, NULL};
  size_t capacity          = 1;
  size_t allocation_size   = 1;

  assert(ngx_http_otel_header_allocation_size((size_t)INT_MAX + 1U, &allocation_size) == NGX_ERROR);
  assert(ngx_http_otel_header_list_capacity(&zero, &capacity, &allocation_size) == NGX_OK);
  assert(capacity == 0);
  assert(allocation_size == 0);
  assert(ngx_http_otel_header_list_capacity(&overflow, &capacity, &allocation_size) == NGX_ERROR);
}

int main(void)
{
  test_repeated_headers(25);
  test_repeated_headers(400);
  test_no_propagation_headers();
  test_trust_incoming_spans_disabled();
  test_allocation_failure_invariant();
  test_capacity_guard();
  test_zero_and_overflow_capacity();
  puts("propagation header tests passed");
  return 0;
}
