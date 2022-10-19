// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/exporters/geneva/metrics/macros.h"
#include "opentelemetry/exporters/geneva/metrics/socket_tools.h"
#include "opentelemetry/ext/http/common/url_parser.h"
#include "opentelemetry/version.h"
#include <memory>
#include <string>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace geneva {
namespace metrics {
constexpr char kSemicolon = ';';
constexpr char kEqual = '=';
constexpr char kEndpoint[] = "Endpoint";
constexpr char kAccount[] = "Account";
constexpr char kNamespace[] = "Namespace";

enum class TransportProtocol { kETW, kTCP, kUDP, kUNIX, kUnknown };

class ConnectionStringParser {

public:
  ConnectionStringParser(const std::string &connection_string)
      : account_(""), namespace_(""),
        url_(nullptr), transport_protocol_{TransportProtocol::kUnknown} {
    std::string::size_type key_pos = 0;
    std::string::size_type key_end;
    std::string::size_type val_pos;
    std::string::size_type val_end;
    bool is_endpoint_found = false;
    while ((key_end = connection_string.find(kEqual, key_pos)) !=
           std::string::npos) {
      if ((val_pos = connection_string.find_first_not_of(kEqual, key_end)) ==
          std::string::npos) {
        break;
      }
      val_end = connection_string.find(kSemicolon, val_pos);
      auto key = connection_string.substr(key_pos, key_end - key_pos);
      auto value = connection_string.substr(val_pos, val_end - val_pos);
      key_pos = val_end;
      if (key_pos != std::string::npos) {
        ++key_pos;
      }
      if (key == kNamespace) {
        namespace_ = value;
      } else if (key == kAccount) {
        account_ = value;
      } else if (key == kEndpoint) {
        is_endpoint_found = true;
        url_ = std::unique_ptr<ext::http::common::UrlParser>(
            new ext::http::common::UrlParser(value));
        if (url_->success_) {
#ifdef HAVE_UNIX_DOMAIN
          if (url_->scheme_ == "unix") {
            transport_protocol_ = TransportProtocol::kUNIX;
          }
#else
          if (url_->scheme_ == "unix") {
            LOG_ERROR("Unix domain socket not supported on this platform")
          }
#endif
          if (url_->scheme_ == "tcp") {
            transport_protocol_ = TransportProtocol::kTCP;
          }
          if (url_->scheme_ == "udp") {
            transport_protocol_ = TransportProtocol::kUDP;
          }
        }
      }
    }
#if defined(_MSC_VER)
    if (account_.size() && namespace_.size() && !is_endpoint_found) {
      transport_protocol_ = TransportProtocol::kETW;
    }
#endif
  }

  bool IsValid() { return transport_protocol_ != TransportProtocol::kUnknown; }

  std::string account_;
  std::string namespace_;
  std::unique_ptr<ext::http::common::UrlParser> url_;
  TransportProtocol transport_protocol_;
};
} // namespace metrics
} // namespace geneva
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE