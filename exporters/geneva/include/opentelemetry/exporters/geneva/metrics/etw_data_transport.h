// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/exporters/geneva/metrics/connection_string_parser.h"
#include "opentelemetry/exporters/geneva/metrics/data_transport.h"
#include "opentelemetry/version.h"

#include <evntprov.h>
#include <memory>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace geneva {
namespace metrics {
class ETWDataTransport : public DataTransport {
public:
  ETWDataTransport(const std::string &etw_provider);
  bool Connect() noexcept override;
  bool Send(const char *data, uint16_t length) noexcept override;
  bool Disconnect() noexcept override;

private:
  REGHANDLE provider_handle_ ;
  bool connected_{false};
};
} // namespace metrics
} // namespace geneva
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE