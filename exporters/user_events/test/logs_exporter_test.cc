// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_LOGS_PREVIEW

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "opentelemetry/exporters/userevents/logs/exporter.h"

#include <gtest/gtest.h>

namespace sdk_logs=opentelemetry::sdk::logs;
namespace userevents_logs=opentelemetry::exporter::userevents::logs;

// Test that when OStream Log exporter is shutdown, no logs should be sent to stream
TEST(UserEventsLogRecordExporter, Shutdown)
{
  auto exporter =
      std::unique_ptr<sdklogs::LogRecordExporter>(new userevents_logs::Exporter({}));

  // Save cout's original buffer here
  std::streambuf *original = std::cout.rdbuf();

  // Redirect cout to our stringstream buffer
  std::stringstream output;
  std::cout.rdbuf(output.rdbuf());

  EXPECT_TRUE(exporter->Shutdown());
}

#endif