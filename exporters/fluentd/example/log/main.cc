// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/fluentd/log/fluentd_exporter.h"
#include "opentelemetry/logs/provider.h"
#include "opentelemetry/sdk/logs/logger_provider.h"
#include "opentelemetry/sdk/logs/simple_log_record_processor.h"

// Using an exporter that simply dumps span data to stdout.
#include "foo_library/foo_library.h"

namespace sdk_logs = opentelemetry::sdk::logs;
namespace nostd = opentelemetry::nostd;

#ifndef HAVE_CONSOLE_LOG
#define HAVE_CONSOLE_LOG
#endif

namespace {
void initLogger() {
  opentelemetry::exporter::fluentd::common::FluentdExporterOptions options;
  options.endpoint = "tcp://localhost:24222";
  auto exporter = std::unique_ptr<sdk_logs::LogRecordExporter>(
      new opentelemetry::exporter::fluentd::logs::FluentdExporter(options));
  auto processor = std::unique_ptr<sdk_logs::LogRecordProcessor>(
      new sdk_logs::SimpleLogRecordProcessor(std::move(exporter)));

  auto provider = std::shared_ptr<opentelemetry::logs::LoggerProvider>(
      new opentelemetry::sdk::logs::LoggerProvider(std::move(processor)));

  auto pr =
      static_cast<opentelemetry::sdk::logs::LoggerProvider *>(provider.get());

  // Set the global logger provider
  opentelemetry::logs::Provider::SetLoggerProvider(provider);
}
} // namespace

int main() {
  // Removing this line will leave the default noop TracerProvider in place.
  initLogger();

  foo_library();
}
