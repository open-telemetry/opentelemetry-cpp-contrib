// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#define ENABLE_LOGS_PREVIEW
#include "opentelemetry/logs/provider.h"

namespace logs = opentelemetry::logs;
namespace nostd = opentelemetry::nostd;

namespace {
nostd::shared_ptr<logs::Logger> get_logger() {
  auto provider = logs::Provider::GetLoggerProvider();
  return provider->GetLogger("foo_library", "", "foo_library");
}

void f1() {
  get_logger()->EmitLogRecord(opentelemetry::logs::Severity::kDebug,
    opentelemetry::common::MakeAttributes({{"k1", "v1"}, {"k2", "v2"}}), std::chrono::system_clock::now());
}

void f2() {
  get_logger()->EmitLogRecord(opentelemetry::logs::Severity::kDebug, nostd::string_view("log body - 3"), std::chrono::system_clock::now());

  f1();
  f1();
}
} // namespace

void foo_library() {
  get_logger()->EmitLogRecord(opentelemetry::logs::Severity::kDebug, nostd::string_view("log body - 1"), std::chrono::system_clock::now());

}
