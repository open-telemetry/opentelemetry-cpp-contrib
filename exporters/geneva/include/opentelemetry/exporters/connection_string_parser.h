// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "opentelemetry/exporters/geneva/metrics/exporter_options.h"
#include "opentelemetry/ext/http/common/url_parser.h"
#include "opentelemetry/version.h"
#include <string>
#include <memory>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace geneva
{
namespace metrics
{
constexpr char kSemicolon = ';';
constexpr char kEqual = '=';
constexpr char kEndpoint[] = "Endpoint";
constexpr char kAccount[] = "Account";
constexpr char kNamespace[] = "Namespace";

enum class TransportProtocol{
    kEtw,
    kTcp,
    kUdp,
    kUnix,
    kUnknown
};

class ConnectionStringParser {

public:
    ConnectionStringParser(const ExporterOptions& options):
    account_(""), namespace_(""), url_(nullptr)
    {
        auto &connection_string = options.connection_string;
        std::string::size_type key_pos = 0;
        std::string::size_type key_end;
        std::string::size_type val_pos;
        std::string::size_type val_end;
        while ((key_end = connection_string.find(kSemicolon, key_pos)) != std::string::npos){
            if (val_pos = connection_string.find_first_not_of(std::string(1, kSemicolon), key_end) == std::string::npos)
            {
                break;
            }
            val_end = connection_string.find(kEqual, val_pos);
            auto key = connection_string.substr(key_pos, key_end - key_pos);
            auto value = connection_string.substr(val_pos, val_end - val_pos);
            if (key == kNamespace){
                namespace_ = value;
            } else if (key == kAccount) {
                account_ = value;
            } else if (key == kEndpoint) {
                url_ = std::unique_ptr<ext::http::common::UrlParser>(new ext::http::common::UrlParser(value));
            }
        }
    }

    bool IsValid(){

    }
private:
    std::string account_;
    std::string namespace_;
    std::unique_ptr<ext::http::common::UrlParser> url_;
    TransportProtocol transport_protocol_;
 };
}
}
}
OPENTELEMETRY_END_NAMESPACE