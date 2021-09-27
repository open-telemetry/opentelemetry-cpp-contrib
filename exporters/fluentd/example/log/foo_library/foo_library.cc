// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#define ENABLE_LOGS_PREVIEW
#include "opentelemetry/logs/provider.h"

namespace logs = opentelemetry::logs;
namespace nostd = opentelemetry::nostd;

namespace {
nostd::shared_ptr<logs::Logger> get_logger() {
  auto provider = logs::Provider::GetLoggerProvider();
  return provider->GetLogger("foo_library");
}

void f1() { get_logger()->Log(opentelemetry::logs::Severity::kDebug, "f1"); }

void f2() {
  get_logger()->Log(opentelemetry::logs::Severity::kDebug, "f2");

  f1();
  f1();
}
} // namespace

void foo_library() {
  get_logger()->Log(opentelemetry::logs::Severity::kDebug, "foo_library");
}
