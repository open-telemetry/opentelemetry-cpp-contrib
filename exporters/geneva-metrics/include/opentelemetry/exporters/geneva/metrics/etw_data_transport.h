// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/exporters/geneva/metrics/connection_string_parser.h"
#include "opentelemetry/exporters/geneva/metrics/data_transport.h"
#include "opentelemetry/version.h"

#include <evntprov.h>
#include <guiddef.h>
#include <memory>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace geneva {
namespace metrics {
static const REGHANDLE INVALID_HANDLE = _UI64_MAX;
static const GUID kMDMProviderGUID = {
    0xedc24920, 0xe004, 0x40f6, 0xa8, 0xe1, 0x0e, 0x6e, 0x48, 0xf3, 0x9d, 0x84};

class ETWDataTransport : public DataTransport {
public:
  ETWDataTransport(const size_t offset_to_skip_);
  bool Connect() noexcept override;
  bool Send(MetricsEventType event_type, const char *data,
            uint16_t length) noexcept override;
  bool Disconnect() noexcept override;
  ~ETWDataTransport();

private:
  REGHANDLE provider_handle_;
  bool connected_{false};
  const size_t offset_to_skip_;
};
} // namespace metrics
} // namespace geneva
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE