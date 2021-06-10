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

#include "opentelemetry.h"

#include <fstream>
#include <memory>

#include "opentelemetry/exporters/ostream/span_exporter.h"
#include "opentelemetry/exporters/otlp/otlp_exporter.h"
#include "opentelemetry/sdk/trace/batch_span_processor.h"
#include "opentelemetry/sdk/trace/simple_processor.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/provider.h"

namespace httpd_otel
{

// TODO: use semantic conventions  https://github.com/open-telemetry/opentelemetry-cpp/issues/566
const nostd::string_view kAttrHTTPServerName         = "http.server_name";
const nostd::string_view kAttrHTTPMethod             = "http.method";
const nostd::string_view kAttrHTTPScheme             = "http.scheme";
const nostd::string_view kAttrHTTPHost               = "http.host";
const nostd::string_view kAttrHTTPTarget             = "http.target";
const nostd::string_view kAttrHTTPUrl                = "http.url";
const nostd::string_view kAttrHTTPFlavor             = "http.flavor";
const nostd::string_view kAttrHTTPClientIP           = "http.client_ip";
const nostd::string_view kAttrNETPeerIP              = "net.peer.ip";
const nostd::string_view kAttrHTTPStatusCode         = "http.status_code";
const nostd::string_view kAttrHTTPResponseContentLen = "http.response_content_length";

OtelConfig config;

void initTracer()
{
  if (!config.fname.empty())
  {
    config.output_file.open(config.fname.c_str(), std::ios::app);
    if (!config.output_file)
    {
      return;  // Error opening OTel output file
    }
  }

  std::unique_ptr<sdktrace::SpanExporter> exporter;
  switch (config.type)
  {
    default:  // suppress warning
      break;
    case OtelExporterType::OSTREAM:
      exporter = std::unique_ptr<sdktrace::SpanExporter>(
        new opentelemetry::exporter::trace::OStreamSpanExporter(
            config.fname.empty() ? std::cerr : config.output_file));
      break;
    case OtelExporterType::OTLP:
      opentelemetry::exporter::otlp::OtlpExporterOptions opts;
      opts.endpoint = config.endpoint;
      exporter      = std::unique_ptr<sdktrace::SpanExporter>(
        new opentelemetry::exporter::otlp::OtlpExporter(opts));
      break;
  }

  std::unique_ptr<sdktrace::SpanProcessor> processor;

  if (config.batch_opts.max_queue_size)
  {
    processor = std::unique_ptr<sdktrace::SpanProcessor>(
      new sdktrace::BatchSpanProcessor(std::move(exporter), config.batch_opts));
  }
  else
  {
    processor = std::unique_ptr<sdktrace::SpanProcessor>(
      new sdktrace::SimpleSpanProcessor(std::move(exporter)));
  }

  // add custom-configured resources
  opentelemetry::sdk::resource::ResourceAttributes resAttrs;
  for(auto &it:config.resources)
  {
    resAttrs[it.first] = it.second;
  }

  auto provider = opentelemetry::v0::nostd::shared_ptr<opentelemetry::trace::TracerProvider>(
    new sdktrace::TracerProvider(std::move(processor),
    opentelemetry::sdk::resource::Resource::Create(resAttrs))
  );

  // Set the global trace provider
  opentelemetry::trace::Provider::SetTracerProvider(provider);
}

opentelemetry::v0::nostd::shared_ptr<opentelemetry::trace::Tracer> get_tracer()
{
  auto provider = opentelemetry::trace::Provider::GetTracerProvider();
  return provider->GetTracer(KHTTPDOTelTracerName);
}

void ExtraRequestData::StartSpan(const HttpdStartSpanAttributes& attrs)
{
  startAttrs = attrs;
  span->SetAttribute(kAttrHTTPServerName, startAttrs.server_name);
  if (!startAttrs.method.empty())
  {
    span->SetAttribute(kAttrHTTPMethod, startAttrs.method);
  }
  if (!startAttrs.scheme.empty())
  {
    span->SetAttribute(kAttrHTTPScheme, startAttrs.scheme);
  }
  if (!startAttrs.host.empty())
  {
    span->SetAttribute(kAttrHTTPHost, startAttrs.host);
  }
  if (!startAttrs.target.empty())
  {
    span->SetAttribute(kAttrHTTPTarget, startAttrs.target);
  }
  if (!startAttrs.url.empty())
  {
    span->SetAttribute(kAttrHTTPUrl, startAttrs.url);
  }
  if (!startAttrs.flavor.empty())
  {
    span->SetAttribute(kAttrHTTPFlavor, startAttrs.flavor);
  }
  span->SetAttribute(kAttrHTTPClientIP, startAttrs.client_ip);
  if (attrs.net_ip != startAttrs.client_ip)
  {
    span->SetAttribute(kAttrNETPeerIP, startAttrs.net_ip);
  }
  // add custom-configured attributes
  for(auto &it:config.attributes)
  {
    span->SetAttribute(it.first, it.second);
  }
}

void ExtraRequestData::EndSpan(const HttpdEndSpanAttributes& attrs)
{
  span->SetAttribute(kAttrHTTPStatusCode, attrs.status);
  span->SetAttribute(kAttrHTTPResponseContentLen, attrs.bytes_sent);
}

}  // namespace httpd_otel
