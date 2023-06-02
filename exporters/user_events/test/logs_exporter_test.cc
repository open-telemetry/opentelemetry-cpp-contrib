// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_LOGS_PREVIEW

#  include <iostream>
#  include <map>
#  include <string>
#  include <vector>

#  include "opentelemetry/exporters/user_events/logs/exporter.h"

#  include <gtest/gtest.h>

namespace sdk_logs         = opentelemetry::sdk::logs;
namespace user_events_logs = opentelemetry::exporter::user_events::logs;

// Test that when OStream Log exporter is shutdown, no logs should be sent to stream
TEST(UserEventsLogRecordExporter, Shutdown)
{
  auto options = user_events_logs::ExporterOptions();
  auto exporter =
      std::unique_ptr<sdk_logs::LogRecordExporter>(new user_events_logs::Exporter(options));

  // Save cout's original buffer here
  std::streambuf *original = std::cout.rdbuf();

  // Redirect cout to our stringstream buffer
  std::stringstream output;
  std::cout.rdbuf(output.rdbuf());

  EXPECT_TRUE(exporter->Shutdown());
}

#endif