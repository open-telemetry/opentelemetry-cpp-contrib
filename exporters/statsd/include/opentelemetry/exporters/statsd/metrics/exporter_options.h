// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

#include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace statsd {
namespace metrics {

// Tags are used to represent dimensions in the metric data.
// We support for different style of tags
enum class TagStyle {
  // For Librato-style tags, they must be appended to the metric name with a delimiting #,
  // as so: "foo#tag1=bar,tag2=baz:100|c"
  Librato,

  // For Geneva-style tags, they must be serialized in JSON format,
  // and appended to the metric name as a JSON object, with the metric name and account and namespace as keys,
  // as so: { "Metric": "TestMetricName", "Account": "TestAccount", "Namespace": "TestNamespace", "Dims": { "Dimension1Name": "Dim1Value", "Dimension2Name": "Dim2Value" }, "TS": "2018-01-01T01:02:03.004" }
  Geneva,

  // The simplest way is not to use tags at all, and just send the metric name and value.
  // This is the default style.
  None
};

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

  const std::map<std::string, std::string> prepopulated_dimensions;

  TagStyle tag_style{TagStyle::None};
};
} // namespace metrics
} // namespace statsd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE