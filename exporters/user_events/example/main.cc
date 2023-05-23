// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_LOGS_PREVIEW

#  include "opentelemetry/exporters/user_events/logs/exporter.h"
#  include "opentelemetry/logs/provider.h"
#  include "opentelemetry/sdk/logs/logger_provider_factory.h"
#  include "opentelemetry/sdk/logs/simple_log_record_processor_factory.h"

#  include "foo_library.h"

namespace logs_api      = opentelemetry::logs;
namespace logs_sdk      = opentelemetry::sdk::logs;
namespace user_events_logs = opentelemetry::exporter::user_events::logs;

namespace
{

void InitLogger()
{
  // Create user_events log exporter instance
  auto exporter_options = user_events_logs::ExporterOptions();
  auto exporter =
      std::unique_ptr<user_events_logs::Exporter>(new user_events_logs::Exporter(exporter_options));
  auto processor = logs_sdk::SimpleLogRecordProcessorFactory::Create(std::move(exporter));
  std::shared_ptr<logs_api::LoggerProvider> provider(
      logs_sdk::LoggerProviderFactory::Create(std::move(processor)));

  // Set the global logger provider
  logs_api::Provider::SetLoggerProvider(provider);
}

void CleanupLogger()
{
  std::shared_ptr<logs_api::LoggerProvider> none;
  logs_api::Provider::SetLoggerProvider(none);
}

}  // namespace

int main()
{
  InitLogger();

  foo_library();

  CleanupLogger();
}
#else
int main()
{
  return 0;
}
#endif  // ENABLE_LOGS_PREVIEW
