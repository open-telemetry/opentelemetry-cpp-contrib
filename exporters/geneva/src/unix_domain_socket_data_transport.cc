// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/geneva/metrics/unix_domain_socket_data_transport.h"
#include "opentelemetry/exporters/geneva/metrics/macros.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace geneva {
namespace metrics {

UnixDomainSocketDataTransport::UnixDomainSocketDataTransport(
    const std::string &connection_string) 
{      
  addr_.reset(new SocketTools::SocketAddr(connection_string.c_str(), true));
}

bool UnixDomainSocketDataTransport::Connect() noexcept {
  if (!connected_) {
    socket_ = SocketTools::Socket(socketparams_);
    connected_ = socket_.connect(*addr_);
    if (!connected_) {
      LOG_ERROR("Geneva Exporter: UDS::Connect failed");
      return false;
    }
  }
  return true;
}

bool UnixDomainSocketDataTransport::Send(MetricsEventType event_type,
                                         char const *data,
                                         uint16_t length) noexcept {
  int error_code = 0;
  if (connected_) {
    socket_.getsockopt(SOL_SOCKET, SO_ERROR, error_code);
  } else {
    LOG_WARN(
        "Geneva Exporter: UDS::Send Socket disconnected - Trying to connect");
    auto status = Connect();
    if (!status) {
      LOG_WARN(
          "Geneva Exporter: UDS::Send Socket reconnect failed. Send failed");
    }
  }
  if (error_code != 0) {
    LOG_ERROR("Geneva Exporter: UDS::Send failed - not connected");
    connected_ = false;
  }

  // try to write
  size_t sent_size = socket_.writeall(data, length);
  if (length == sent_size) {
    // Disconnect();
    return true;
  } else {
    LOG_ERROR("Geneva Exporter: UDS::Send failed");
  }
  return false;
}

bool UnixDomainSocketDataTransport::Disconnect() noexcept {
  if (connected_) {
    connected_ = false;
    if (socket_.invalid()) {
      socket_.close();
      return true;
    }
  }
  LOG_WARN("Geneva Exporter: Already disconnected");
  return false;
}
} // namespace metrics
} // namespace geneva
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE