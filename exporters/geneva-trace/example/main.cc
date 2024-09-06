// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/logs/provider.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/span.h"
#include "opentelemetry/exporters/geneva/geneva_logger_exporter.h"
#include "opentelemetry/exporters/geneva/geneva_tracer_exporter.h"

#include <string>

const char *kGlobalProviderName = "OpenTelemetry-ETW-TLD-Geneva-Example";
std::string providerName = kGlobalProviderName;

using L = std::vector<std::pair<trace_api::SpanContext, std::map<std::string, std::string>>>;

namespace
{
auto InitTracer()
{
  static opentelemetry::exporter::etw::TracerProvider tracer_provider;
  auto tracer = tracer_provider.GetTracer(providerName, "1.0");

  return tracer;
}
auto InitLogger()
{
  static opentelemetry::exporter::etw::LoggerProvider logger_provider;
  auto logger = logger_provider.GetLogger(providerName, "1.0");

  return logger;
}
}

int main()
{
  auto tracer = InitTracer();
  auto logger = InitLogger();

  auto s1 = tracer->StartSpan("main");

  {
    L link1 = {{s1->GetContext(), {}}};

    // Create Span with 1 SpanLink
    auto s2 = tracer->StartSpan("child", opentelemetry::common::MakeAttributes({{"key1", "value 1"}, {"key2", 1}}), link1);

    s2->SetAttribute("attr_key1", 123);

    {
      L link2 = {{s1->GetContext(), {}}, {s2->GetContext(), {}}};

      // Create Span with 2 SpanLinks
      auto s3 = tracer->StartSpan("grandchild", opentelemetry::common::MakeAttributes({{"key3", "value 3"}, {"key4", 2}}), link2);

      s3->SetAttribute("attr_key2", 456);

      s3->End();
    }

    s2->End();
  }

  s1->End();

  logger->Info("Hello World!");

  return 0;
}