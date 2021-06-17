/*
 * Copyright The OpenTelemetry Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HTTPD_OPENTELEMETRY_H_
#define HTTPD_OPENTELEMETRY_H_

#include <fstream>
#include <unordered_map>

#include "opentelemetry/context/context.h"
#include "opentelemetry/nostd/string_view.h"
#include "opentelemetry/sdk/trace/batch_span_processor.h"
#include "opentelemetry/trace/provider.h"

namespace httpd_otel
{

const opentelemetry::nostd::string_view KHTTPDOTelTracerName = "httpd";

enum class OtelExporterType
{
  NONE    = 0,
  OSTREAM = 1,
  OTLP    = 2
};

enum class OtelPropagation
{
  NONE             = 0,
  TRACE_CONTEXT    = 1,
  B3_SINGLE_HEADER = 2,
  B3_MULTI_HEADER  = 3
};

struct OtelConfig
{
  OtelExporterType type;
  std::string fname;     // for exporter type ostream
  std::string endpoint;  // for exporter type otlp
  std::ofstream output_file;
  // configuration for batch processing
  opentelemetry::sdk::trace::BatchSpanProcessorOptions batch_opts;
  // context propagation
  bool ignore_inbound;
  OtelPropagation propagation;
  std::unordered_map<std::string, std::string> attributes;
  std::unordered_map<std::string, std::string> resources;
  OtelConfig() : ignore_inbound(true) {}
};

extern OtelConfig config;

struct HttpdStartSpanAttributes
{
  std::string server_name;
  std::string method;
  std::string scheme;
  std::string host;
  std::string target;
  std::string url;  // as an alternative for scheme + host + target
  std::string flavor;
  std::string client_ip;
  std::string net_ip;
};

struct HttpdEndSpanAttributes
{
  int status;
  int64_t bytes_sent;
};

// this object is created using placement new therefore all destructors needs
// to be called explictly inside Destruct method
struct ExtraRequestData
{  // context which we pass
  opentelemetry::nostd::unique_ptr<opentelemetry::context::Token> token;
  opentelemetry::nostd::shared_ptr<opentelemetry::v0::trace::Span> span;
  HttpdStartSpanAttributes startAttrs;
  HttpdEndSpanAttributes endAttrs;
  // Sets attributes for HTTP request.
  void StartSpan(const HttpdStartSpanAttributes &attrs);
  void EndSpan(const HttpdEndSpanAttributes &attrs);
  // we are called from apache when apr_pool_t is being cleaned after request
  // due to fact that callback is "C" style we are receiving This as 1st argument
  static int Destruct(ExtraRequestData *This)
  {
    if (This->token)
    {
      This->token.release();
      This->token = nullptr;
    }
    if (This->span)
    {
      This->span->End();
      This->span = nullptr;
    }
    return 0;  // return value (apr_status_t) is ignored by run_cleanups
  }
};

// Initializes OpenTelemetry module.
void initTracer();
// Returns default (global) tracer.
opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> get_tracer();

}  // namespace httpd_otel

#endif  // HTTPD_OPENTELEMETRY_H_
