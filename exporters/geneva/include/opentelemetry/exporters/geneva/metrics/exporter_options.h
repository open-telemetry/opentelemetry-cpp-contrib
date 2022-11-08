// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/version.h"
#include <string>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace geneva {
namespace metrics {

struct ExporterOptions {
  // clang-format off
  /*
  Format -
    Windows:
        Account={MetricAccount};NameSpace={MetricNamespace}
    Linux:
        Endpoint=unix://{UDS Path};Account={MetricAccount};Namespace={MetricNamespace}
  */
// clang-format off
  std::string connection_string;
};
} // namespace metrics
} // namespace geneva
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE