// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace user_events
{
namespace logs
{

const std::string kDefaultUserEventsLogsProviderName = "opentelemetry_logs";

/**
 * Struct to hold the options needed for the user_events logs exporter.
 */

struct ExporterOptions
{
public:
    ExporterOptions() : ExporterOptions(kDefaultUserEventsLogsProviderName) {}

    ExporterOptions(std::string provider_name)
    {
        if (provider_name.empty())
        {
            this->provider_name = kDefaultUserEventsLogsProviderName;
        }
        else
        {
            this->provider_name = provider_name;
        }
    }

public:
    std::string provider_name;
};

}  // namespace logs
}  // namespace user_events
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
