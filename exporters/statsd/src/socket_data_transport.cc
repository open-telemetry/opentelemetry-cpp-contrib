// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/statsd/metrics/connection_string_parser.h"
#include "opentelemetry/exporters/statsd/metrics/socket_data_transport.h"
#include "opentelemetry/exporters/statsd/metrics/macros.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace statsd {
namespace metrics {

SocketDataTransport::SocketDataTransport(
    const ConnectionStringParser &parser) 
{      
  bool is_unix_domain = false;

  if (parser.transport_protocol_ == TransportProtocol::kTCP) {
    socketparams_ = {AF_INET, SOCK_STREAM, 0};
  }
  else if (parser.transport_protocol_ == TransportProtocol::kUDP) {
    socketparams_ = {AF_INET, SOCK_DGRAM, 0};
  }
  else if (parser.transport_protocol_ == TransportProtocol::kUNIX) {
    socketparams_ = {AF_UNIX, SOCK_STREAM, 0};
    is_unix_domain = true;
  }
  else {
    LOG_ERROR("Statsd Exporter: Invalid transport protocol");
  }
  addr_.reset(new SocketTools::SocketAddr(parser.connection_string_.c_str(), is_unix_domain));
}

bool SocketDataTransport::Connect() noexcept {
  if (!connected_) {
    socket_ = SocketTools::Socket(socketparams_);
    connected_ = socket_.connect(*addr_);
    if (!connected_) {
      LOG_ERROR("Statsd Exporter: UDS::Connect failed");
      return false;
    }
  }
  return true;
}

bool SocketDataTransport::Send(MetricsEventType event_type,
                                         char const *data,
                                         uint16_t length) noexcept {
  int error_code = 0;
  if (connected_) {
    socket_.getsockopt(SOL_SOCKET, SO_ERROR, error_code);
  } else {
    LOG_WARN(
        "Statsd Exporter: UDS::Send Socket disconnected - Trying to connect");
    auto status = Connect();
    if (!status) {
      LOG_WARN(
          "Statsd Exporter: UDS::Send Socket reconnect failed. Send failed");
    }
  }
  if (error_code != 0) {
    LOG_ERROR("Statsd Exporter: UDS::Send failed - not connected");
    connected_ = false;
    return false;
  }

  // try to write
  size_t sent_size = socket_.writeall(data, length);
  if (length != sent_size) {
    Disconnect();
    LOG_ERROR("Statsd Exporter: UDS::Send failed");
    return false;
  }
  return true;
}

bool SocketDataTransport::Disconnect() noexcept {
  if (connected_) {
    connected_ = false;
    if (socket_.invalid()) {
      socket_.close();
      return true;
    }
  }
  LOG_WARN("Statsd Exporter: Already disconnected");
  return false;
}
} // namespace metrics
} // namespace statsd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE