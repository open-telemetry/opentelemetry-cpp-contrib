// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#define ENABLE_LOGS_PREVIEW 
#include "opentelemetry/logs/provider.h"

#include <iostream>

namespace logs = opentelemetry::logs;
namespace nostd = opentelemetry::nostd;

namespace
{
nostd::shared_ptr<logs::Logger> get_logger()
{
  auto provider = logs::Provider::GetLoggerProvider();
  if (provider) {
    std::cout << " Provider valid\n" << provider.get() << "\n";
  }
  return provider->GetLogger("foo_library");
}

void f1()
{
  std::cout << "f1\n";
  get_logger()->Log(opentelemetry::logs::Severity::kDebug, "f1");
}

void f2()
{
    std::cout << "f2\n";

 get_logger()->Log(opentelemetry::logs::Severity::kDebug, "f2");

  f1();
  f1();

}
}  // namespace

void foo_library()
{
    std::cout << "foo\n";

 get_logger()->Log(opentelemetry::logs::Severity::kDebug, "foo_library");

}
