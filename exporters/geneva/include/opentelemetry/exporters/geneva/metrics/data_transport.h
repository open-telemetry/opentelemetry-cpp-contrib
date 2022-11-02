// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/version.h"

#include <vector>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace geneva {
namespace metrics {
using ByteVector = std::vector<unsigned char>;

class DataTransport {
public:
  virtual bool Connect() noexcept = 0;
  virtual bool Send(const char *data, uint16_t length) noexcept = 0;
  virtual bool Disconnect() noexcept = 0;
};
} // namespace metrics
} // namespace geneva
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE