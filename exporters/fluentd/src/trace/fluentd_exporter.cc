// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#define HAVE_CONSOLE_LOG

#include "opentelemetry/exporters/fluentd/trace/fluentd_exporter.h"
#include "opentelemetry/exporters/fluentd/trace/recordable.h"
#include "opentelemetry/ext/http/common/url_parser.h"

#include "opentelemetry/exporters/fluentd/common/fluentd_logging.h"

#include "nlohmann/json.hpp"

#include <cassert>

using UrlParser = opentelemetry::ext::http::common::UrlParser;

using namespace nlohmann;

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace fluentd {
namespace trace {

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
std::unique_ptr<sdk::trace::Recordable>
FluentdExporter::MakeRecordable() noexcept {
  return std::unique_ptr<sdk::trace::Recordable>(new Recordable);
}

/**
 * @brief Export spans.
 * @param spans
 * @return Export result.
 */
sdk::common::ExportResult FluentdExporter::Export(
    const nostd::span<std::unique_ptr<sdk::trace::Recordable>>
        &spans) noexcept {

  // Return failure if this exporter has been shutdown
  if (is_shutdown_) {
    return sdk::common::ExportResult::kFailure;
  }

  // If no spans in Recordable, then return error.
  if (spans.size() == 0) {
    return sdk::common::ExportResult::kFailure;
  }

  json events = {};
  {
    // Write all Spans first
    json obj = json::array();
    obj.push_back(FLUENT_VALUE_SPAN);
    json spanevents = json::array();
    for (auto &recordable : spans) {
      auto rec = std::unique_ptr<Recordable>(
          static_cast<Recordable *>(recordable.release()));
      if (rec != nullptr) {
        auto span = rec->span();
        // Emit "Span" as fluentd event
        json record = json::array();
        record.push_back(span["options"][FLUENT_FIELD_ENDTTIME]);
        json fields = {};
        for (auto &kv : span["options"].items()) {
          fields[kv.key()] = kv.value();
        }
        record.push_back(fields);
        spanevents.push_back(record);
        // Iterate over all events added on this span
        for (auto &v : span["events"]) {
          auto &event = v[1];
          std::string name = event[FLUENT_FIELD_NAME];

          event[FLUENT_FIELD_SPAN_ID] = span["options"][FLUENT_FIELD_SPAN_ID];
          event[FLUENT_FIELD_TRACE_ID] = span["options"][FLUENT_FIELD_TRACE_ID];

          // Event Time flows as a separate field in fluentd.
          // However, if we may need to consider addding
          // span["options"][FLUENT_FIELD_TIME]
          //
          // Complete list of all Span attributes :
          // for (auto &kv : span["options"].items()) { ... }

          // Group all events by matching event name in array.
          // This array is translated into FluentD forward payload.
          if (!events.contains(name)) {
            events[name] = json::array();
          }
          events[name].push_back(v);
        }
      }
    }
    obj.push_back(spanevents);
    LOG_TRACE("sending %zu Span event(s)", obj[1].size());
    std::vector<uint8_t> msg = nlohmann::json::to_msgpack(obj);
    // Immediately send the Span event(s)
    bool result = Send(msg);
    if (!result) {
      return sdk::common::ExportResult::kFailure;
    }
  }
  if (options_.convert_event_to_trace) {
    for (auto &kv : events.items()) {
      json obj = json::array();
      obj.push_back(kv.key());
      json otherevents = json::array();
      for (auto &v : kv.value()) {
        otherevents.push_back(v);
      }
      obj.push_back(otherevents);
      LOG_TRACE("sending %zu %s events", obj[1].size(), kv.key().c_str());

      std::vector<uint8_t> msg = nlohmann::json::to_msgpack(obj);
      // Immediately send the Span event(s)
      bool result = Send(msg);
      if (!result) {
        return sdk::common::ExportResult::kFailure;
      }
    }
  }

  // At this point we always return success because there is no way
  // to know if delivery is gonna succeed with multiple retries.
  return sdk::common::ExportResult::kSuccess;
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

    LOG_WARN("send failed, retrying %u ...", (unsigned int)retryCount);
    // Retry to connect and/or send
  }

  LOG_ERROR("send failed!");
  return false;
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
 * @brief Shutdown FluentD exporter
 * @param
 * @return
 */
bool FluentdExporter::Shutdown(std::chrono::microseconds) noexcept {

  is_shutdown_ = true;
  return true;
}

} // namespace trace
} // namespace fluentd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
