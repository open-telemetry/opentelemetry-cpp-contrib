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

void f1() {
  get_logger()->Log(opentelemetry::logs::Severity::kDebug, "logName2",
                    {{"k1", "v1"}, {"k2", "v2"}});
}

void f2() {
  get_logger()->Log(opentelemetry::logs::Severity::kDebug, "logName3",
                    "log body - 3");

  f1();
  f1();
}
} // namespace

void foo_library() {
  get_logger()->Log(opentelemetry::logs::Severity::kDebug, "logName1",
                    "log body - 1");
}
