// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/logs/provider.h"

namespace logs = opentelemetry::logs;
namespace nostd = opentelemetry::nostd;

namespace {
nostd::shared_ptr<logs::Logger> get_logger() {
  auto provider = logs::Provider::GetLoggerProvider();
  return provider->GetLogger("foo_library", "", "foo_library");
}

void f1() {
  auto log_record = get_logger()->CreateLogRecord();
  log_record->SetSeverity(opentelemetry::logs::Severity::kDebug);
  log_record->SetAttribute("k1", "v1");
  log_record->SetAttribute("k2", "v2");
  log_record->SetTimestamp(std::chrono::system_clock::now());
  log_record->SetEventId(2, "logName2");

  get_logger()->EmitLogRecord(std::move(log_record));
}

void f2() {
  auto log_record = get_logger()->CreateLogRecord();
  log_record->SetSeverity(opentelemetry::logs::Severity::kDebug);
  log_record->SetBody("log body - 3");
  log_record->SetTimestamp(std::chrono::system_clock::now());
  log_record->SetEventId(3, "logName3");

  get_logger()->EmitLogRecord(std::move(log_record));

  f1();
  f1();
}
} // namespace

void foo_library() {
  auto log_record = get_logger()->CreateLogRecord();
  log_record->SetSeverity(opentelemetry::logs::Severity::kDebug);
  log_record->SetBody("log body - 1");
  log_record->SetTimestamp(std::chrono::system_clock::now());
  log_record->SetEventId(1, "logName1");

  get_logger()->EmitLogRecord(std::move(log_record));
}
