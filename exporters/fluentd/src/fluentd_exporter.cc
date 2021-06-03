/*
 * Copyright The OpenTelemetry Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "opentelemetry/exporters/fluentd/fluentd_exporter.h"
#include "opentelemetry/exporters/fluentd/recordable.h"
#include "opentelemetry/ext/net/common/url_parser.h"

#include "nlohmann/json.hpp"

using UrlParser = opentelemetry::ext::net::common::UrlParser;

using namespace nlohmann;

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace fluentd {

// -------------------------------- Constructors
// --------------------------------

FluentdExporter::FluentdExporter(const FluentdExporterOptions& options)
    : options_(options) {
  Initialize();
}

FluentdExporter::FluentdExporter() : options_(FluentdExporterOptions()) {
  Initialize();
}

// ----------------------------- Exporter methods ------------------------------

std::unique_ptr<sdk::trace::Recordable>
FluentdExporter::MakeRecordable() noexcept {
  return std::unique_ptr<sdk::trace::Recordable>(new Recordable);
}

sdk::common::ExportResult FluentdExporter::Export(
    const nostd::span<std::unique_ptr<sdk::trace::Recordable>>&
        spans) noexcept {
  if (isShutdown_) {
    return sdk::common::ExportResult::kFailure;
  }

  if (spans.size() == 0) {
    return sdk::common::ExportResult::kFailure;
  }

  json events = {};

  { // Write all Spans
    json obj = json::array();
    obj.push_back(FLUENT_VALUE_SPAN);
    json spanevents = json::array();
    for (auto& recordable : spans) {
      auto rec = std::unique_ptr<Recordable>(
          static_cast<Recordable*>(recordable.release()));
      if (rec != nullptr) {
        auto span = rec->span();
        // Span event
        json record = json::array();
        record.push_back(span["options"][FLUENT_FIELD_ENDTTIME]);
        json fields = {};
        for (auto& kv : span["options"].items()) {
          fields[kv.key()] = kv.value();
        }
        record.push_back(fields);
        spanevents.push_back(record);

        // Iterate over all events on this span
        for (auto& v : span["events"]) {
          auto& event = v[1];
          std::string name = event[FLUENT_FIELD_NAME];
          for (auto& kv : span["options"].items()) {
            // TODO: capture only necessary fields, not all
            event[kv.key()] = kv.value();
          }
          // group by event name
          if (!events.contains(name)) {
            events[name] = json::array();
          }
          events[name].push_back(v);
        }
      }
    }
    obj.push_back(spanevents);
    std::vector<uint8_t> msg = nlohmann::json::to_msgpack(obj);
    socket_.writeall(msg);
  }

  for (auto &kv : events.items())
  {
    json obj = json::array();
    obj.push_back(kv.key());
    json otherevents = json::array();
    for (auto &v : kv.value())
    {
      otherevents.push_back(v);
    }
    obj.push_back(otherevents);
    std::vector<uint8_t> msg = nlohmann::json::to_msgpack(obj);
    socket_.writeall(msg);
  }

  return sdk::common::ExportResult::kSuccess;
}

void FluentdExporter::Initialize() {
  UrlParser url(options_.endpoint);
  bool isUnixDomain = false;

  if (url.scheme_ == "tcp") {
    socketparams_ = {AF_INET, SOCK_STREAM, 0};
  } else if (url.scheme_ == "udp") {
    socketparams_ = {AF_INET, SOCK_DGRAM, 0};
  }
#ifdef HAVE_UNIX_DOMAIN
  else if (url.scheme_ == "unix") {
    socketparams_ = {AF_UNIX, SOCK_STREAM, 0};
    isUnixDomain = true;
  }
#endif
  else {
    throw new std::runtime_error("Invalid endpoint!");
  }

  addr_.reset(
      new SocketTools::SocketAddr(options_.endpoint.c_str(), isUnixDomain));
  // std::cout << "Connecting to " << addr_->toString().c_str() << std::endl;

  socket_ = SocketTools::Socket(socketparams_);
  bool isConnected = socket_.connect(*addr_);
  if (!isConnected) {
    std::string msg("Unable to connect to ");
    msg += options_.endpoint;
    throw new std::runtime_error(msg);
  }
}

}  // namespace fluentd
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
