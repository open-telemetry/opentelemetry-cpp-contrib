// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#ifndef HAVE_CONSOLE_LOG
#define HAVE_CONSOLE_LOG
#endif

#include "opentelemetry/exporters/fluentd/log/fluentd_exporter.h"
#include "opentelemetry/exporters/fluentd/log/recordable.h"
#include "opentelemetry/ext/http/common/url_parser.h"
#include "opentelemetry/sdk/logs/read_write_log_record.h"

#include "opentelemetry/exporters/fluentd/common/fluentd_logging.h"

#include "nlohmann/json.hpp"

#include <cassert>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace fluentd {
namespace logs {

using UrlParser = opentelemetry::ext::http::common::UrlParser;

using namespace nlohmann;

/**
 * @brief Scheme for tcp:// stream
 */
constexpr const char *kTCP = "tcp";

/**
 * @brief Scheme for udp:// datagram
 */
constexpr const char *kUDP = "udp";

/**
 * @brief Scheme for unix:// domain socket
 */
constexpr const char *kUNIX = "unix";

/**
 * @brief Create FluentD exporter with options
 * @param options
 */
FluentdExporter::FluentdExporter(
    const fluentd_common::FluentdExporterOptions &options)
    : options_(options) {
  Initialize();
}

/**
 * @brief Create FluentD exporter with default options
 */
FluentdExporter::FluentdExporter()
    : options_(fluentd_common::FluentdExporterOptions()) {
  Initialize();
}

/**
 * @brief Create new Recordable
 * @return Recordable
 */
std::unique_ptr<logs_sdk::Recordable>
FluentdExporter::MakeRecordable() noexcept {
  return std::unique_ptr<logs_sdk::Recordable>(new opentelemetry::exporter::fluentd::logs::Recordable());
}

/**
 * @brief Export spans.
 * @param spans
 * @return Export result.
 */
sdk::common::ExportResult FluentdExporter::Export(
    const nostd::span<std::unique_ptr<logs_sdk::Recordable>> &logs) noexcept {
  // Return failure if this exporter has been shutdown
  if (is_shutdown_) {
    return sdk::common::ExportResult::kFailure;
  }

  // If no spans in Recordable, then return error.
  if (logs.size() == 0) {
    return sdk::common::ExportResult::kFailure;
  }
  {
    json obj = json::array();
    obj.push_back(FLUENT_VALUE_LOG);
    json logevents = json::array();
    for (auto &recordable : logs) {
      auto rec = std::unique_ptr<Recordable>(
          static_cast<Recordable *>(recordable.release()));
      if (rec != nullptr) {
        auto log = rec->Log();
        // Emit "log" as fluentd event
        json record = json::array();
        // ObservedTimestamp is now set when Log/EmitLogRecord is invoked rather than Timestamp.
        record.push_back(log[FLUENT_FIELD_OBSERVEDTIMESTAMP]);
        json fields = {};
        for (auto &kv : log.items()) {
          fields[kv.key()] = kv.value();
        }
        record.push_back(fields);
        logevents.push_back(record);
      }
    }
    obj.push_back(logevents);
    LOG_TRACE("sending %zu Span event(s)", obj[1].size());
    std::vector<uint8_t> msg = nlohmann::json::to_msgpack(obj);
    // Immediately send the Span event(s)
    bool result = Send(msg);
    if (!result) {
      return sdk::common::ExportResult::kFailure;
    }
  }
  // At this point we always return success because there is no way
  // to know if delivery is gonna succeed with multiple retries.
  return sdk::common::ExportResult::kSuccess;
}

/**
 * @brief Try to upload fluentd forward protocol packet.
 * This method respects the retry options for connects
 * and upload retries.
 *
 * @param packet
 * @return true if packet got delivered.
 */
bool FluentdExporter::Send(std::vector<uint8_t> &packet) {
  size_t retryCount = options_.retry_count;
  while (retryCount--) {
    int error_code = 0;
    // Check if socket is Okay
    if (connected_) {
      socket_.getsockopt(SOL_SOCKET, SO_ERROR, error_code);
      if (error_code != 0) {
        connected_ = false;
      }
    }
    // Reconnect if not Okay
    if (!connected_) {
      // Establishing socket connection may take time
      if (!Connect()) {
        continue;
      }
      LOG_DEBUG("socket connected");
    }

    // Try to write
    size_t sentSize = socket_.writeall(packet);
    if (packet.size() == sentSize) {
      LOG_DEBUG("send successful");
      Disconnect();
      LOG_DEBUG("socket disconnected");
      return true;
    }

    LOG_WARN("send failed, retrying %lu ...", retryCount);
    // Retry to connect and/or send
  }

  LOG_ERROR("send failed!");
  return false;
}

/**
 * @brief Establish connection to FluentD
 * @return true if connected successfully.
 */
bool FluentdExporter::Connect() {
  if (!connected_) {
    socket_ = SocketTools::Socket(socketparams_);
    connected_ = socket_.connect(*addr_);
    if (!connected_) {
      LOG_ERROR("Unable to connect to %s", options_.endpoint.c_str());
      return false;
    }
  }
  // Connected or already connected
  return true;
}

/**
 * @brief Disconnect FluentD socket or datagram.
 * @return
 */
bool FluentdExporter::Disconnect() {
  if (connected_) {
    connected_ = false;
    if (!socket_.invalid()) {
      socket_.close();
      return true;
    }
  }
  return false;
}

/**
 * @brief Initialize FluentD exporter socket.
 * @return true if end-point settings have been accepted.
 */
bool FluentdExporter::Initialize() {
  UrlParser url(options_.endpoint);
  bool is_unix_domain = false;

  if (url.scheme_ == kTCP) {
    socketparams_ = {AF_INET, SOCK_STREAM, 0};
  } else if (url.scheme_ == kUDP) {
    socketparams_ = {AF_INET, SOCK_DGRAM, 0};
  }
#ifdef HAVE_UNIX_DOMAIN
  else if (url.scheme_ == kUNIX) {
    socketparams_ = {AF_UNIX, SOCK_STREAM, 0};
    is_unix_domain = true;
  }
#endif
  else {
#if defined(__EXCEPTIONS)
    // Customers MUST specify valid end-point configuration
    throw new std::runtime_error("Invalid endpoint!");
#endif
    return false;
  }
  addr_.reset(
      new SocketTools::SocketAddr(options_.endpoint.c_str(), is_unix_domain));
  LOG_TRACE("connecting to %s", addr_->toString().c_str());

  return true;
}

/**
 * @brief Shutdown FluentD exporter
 * @param
 * @return
 */
bool FluentdExporter::Shutdown(std::chrono::microseconds) noexcept {

  is_shutdown_ = true;
  return false;
}

} // namespace logs
} // namespace fluentd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
