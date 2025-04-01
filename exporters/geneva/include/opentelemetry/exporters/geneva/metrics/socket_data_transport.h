// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/exporters/geneva/metrics/connection_string_parser.h"
#include "opentelemetry/exporters/geneva/metrics/data_transport.h"
#include "opentelemetry/exporters/geneva/metrics/socket_tools.h"
#include "opentelemetry/version.h"

#include <memory>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace geneva {
namespace metrics {
class SocketDataTransport : public DataTransport {
public:
  SocketDataTransport(const ConnectionStringParser &parser);
  bool Connect() noexcept override;
  bool Send(MetricsEventType event_type, const char *data,
            uint16_t length) noexcept override;
  bool Disconnect() noexcept override;
  ~SocketDataTransport() = default;

private:
  // Socket connection is re-established for every batch of events
  SocketTools::SocketParams socketparams_{AF_UNIX, SOCK_STREAM, 0};
  SocketTools::Socket socket_;
  std::unique_ptr<SocketTools::SocketAddr> addr_;
  bool connected_{false};
};
} // namespace metrics
} // namespace geneva
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE