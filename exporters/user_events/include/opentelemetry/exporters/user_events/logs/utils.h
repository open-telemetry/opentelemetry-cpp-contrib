// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/sdk/common/attribute_utils.h"
#include "opentelemetry/version.h"

#include <eventheader/EventHeaderDynamic.h>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace user_events
{
namespace utils
{

namespace api_common = opentelemetry::common;

void PopulateAttribute(nostd::string_view key,
                       const api_common::AttributeValue &value,
                       ehd::EventBuilder &event_builder) noexcept;

}  // namespace utils
}  // namespace user_events
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE