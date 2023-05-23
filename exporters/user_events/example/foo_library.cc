// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_LOGS_PREVIEW

#  include <map>
#  include <string>
#  include "opentelemetry/logs/provider.h"
#  include "opentelemetry/sdk/version/version.h"

namespace logs  = opentelemetry::logs;
namespace trace = opentelemetry::trace;
namespace nostd = opentelemetry::nostd;

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

  logger->Trace("body", opentelemetry::common::SystemTimestamp(std::chrono::system_clock::now()));
}

#endif
