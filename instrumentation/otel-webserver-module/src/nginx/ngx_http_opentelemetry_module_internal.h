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

#ifndef NGX_HTTP_OPENTELEMETRY_MODULE_INTERNAL_H
#define NGX_HTTP_OPENTELEMETRY_MODULE_INTERNAL_H

#include <limits.h>
#include <stdint.h>

static ngx_int_t ngx_http_otel_header_allocation_size(size_t count, size_t *allocation_size)
{
  if (count > INT_MAX || count > SIZE_MAX / sizeof(http_headers))
  {
    return NGX_ERROR;
  }

  *allocation_size = count * sizeof(http_headers);
  return NGX_OK;
}

static ngx_int_t ngx_http_otel_header_list_capacity(const ngx_list_part_t *part,
                                                    size_t *capacity,
                                                    size_t *allocation_size)
{
  size_t count = 0;

  for (; part != NULL; part = part->next)
  {
    if ((size_t)part->nelts > SIZE_MAX - count)
    {
      return NGX_ERROR;
    }
    count += (size_t)part->nelts;
  }

  if (ngx_http_otel_header_allocation_size(count, allocation_size) != NGX_OK)
  {
    return NGX_ERROR;
  }

  *capacity = count;
  return NGX_OK;
}

static ngx_int_t ngx_http_otel_fill_propagation_headers(const ngx_list_part_t *part,
                                                        http_headers *propagation_headers,
                                                        size_t capacity,
                                                        ngx_flag_t trust_incoming_spans,
                                                        size_t *populated)
{
  size_t count = 0;

  *populated = 0;

  if (!trust_incoming_spans || propagation_headers == NULL || capacity == 0)
  {
    return NGX_OK;
  }

  for (; part != NULL; part = part->next)
  {
    ngx_table_elt_t *header = (ngx_table_elt_t *)part->elts;

    for (ngx_uint_t j = 0; j < part->nelts; j++)
    {
      ngx_table_elt_t *h = &header[j];

      for (size_t i = 0; i < headers_len; i++)
      {
        if (strcmp((const char *)h->key.data, httpHeaders[i]) == 0)
        {
          if (count >= capacity)
          {
            return NGX_ERROR;
          }
          propagation_headers[count].name  = (char *)httpHeaders[i];
          propagation_headers[count].value = h->value.data == NULL ? "" : (char *)h->value.data;
          count++;
          break;
        }
      }
    }
  }

  *populated = count;
  return NGX_OK;
}

#endif /* NGX_HTTP_OPENTELEMETRY_MODULE_INTERNAL_H */
