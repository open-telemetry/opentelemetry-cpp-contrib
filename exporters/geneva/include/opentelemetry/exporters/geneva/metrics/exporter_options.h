// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/version.h"
#include <string>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace geneva
{
namespace metrics
{

 struct ExporterOptions
 {
    /*
    Format -
      Windows:
          Account={MetricAccount};NameSpace={MetricNamespace}
      Linux:
          Endpoint=unix:{UDS Path};Account={MetricAccount};Namespace={MetricNamespace}
    */
    std::string connection_string ;
 };
}
}
}
OPENTELEMETRY_END_NAMESPACE