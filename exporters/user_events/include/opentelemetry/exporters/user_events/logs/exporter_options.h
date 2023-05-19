// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef ENABLE_LOGS_PREVIEW

#include "opentelemetry/version.h"
#include <string>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace userevents {
namespace logs {

struct ExporterOptions {
};
} // namespace logs
} // namespace userevents
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE

#endif