// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/logs/provider.h"
#include "opentelemetry/exporters/geneva/geneva_logger_exporter.h"

#include <string>

const char *kGlobalProviderName = "OpenTelemetry-ETW-TLD-Geneva-Example";
std::string providerName = kGlobalProviderName;

namespace
{
auto InitLogger()
{
  opentelemetry::exporter::etw::LoggerProvider logger_provider;
  auto logger = logger_provider.GetLogger(providerName, "1.0");

  return logger;
}
}

int main()
{
  auto logger = InitLogger();

  logger->Info("Hello World!");

  return 0;
}