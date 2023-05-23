// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_LOGS_PREVIEW

#  include <string>
#  include "opentelemetry/logs/event_id.h"
#  include "opentelemetry/logs/provider.h"
#  include "opentelemetry/sdk/version/version.h"

namespace logs  = opentelemetry::logs;
namespace nostd = opentelemetry::nostd;

const logs::EventId function_name_event_id{0x12345678, "Company.Component.SubComponent.FunctionName"};

namespace
{
nostd::shared_ptr<logs::Logger> get_logger()
{
  auto provider = logs::Provider::GetLoggerProvider();
  return provider->GetLogger("foo_library_logger", "foo_library");
}
}  // namespace

void foo_library()
{
  auto logger = get_logger();

  logger->Trace(
      function_name_event_id,
      "Simulate function enter trace message from {process_id}:{thread_id}",
      opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));

}

#endif
