// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/user_events/logs/exporter.h"
#include "opentelemetry/logs/provider.h"
#include "opentelemetry/sdk/logs/logger_provider_factory.h"
#include "opentelemetry/sdk/logs/processor.h"
#include "opentelemetry/sdk/logs/simple_log_record_processor_factory.h"

#include <chrono>
#include <thread>

#include "foo_library.h"

namespace logs_api         = opentelemetry::logs;
namespace logs_sdk         = opentelemetry::sdk::logs;
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

using namespace std;

int i = 0;
std::string get_fruit_name_from_customer()
{
  static int i = 0;

  if (i++ % 2 == 0)
  {
    return "strawberry";
  }
  else
  {
    return "apple";
  }
}

int main()
{
  InitLogger();

  while (true)
  {
    string fruit_name = get_fruit_name_from_customer();

    sell_fruit(fruit_name);

    this_thread::sleep_for(chrono::seconds(3));
  }

  CleanupLogger();
}
