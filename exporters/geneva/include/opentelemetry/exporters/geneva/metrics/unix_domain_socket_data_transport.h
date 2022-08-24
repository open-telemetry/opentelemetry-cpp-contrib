// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/exporters/geneva/metrics/connection_string_parser.h"
#include "opentelemetry/exporters/geneva/metrics/data_transport.h"
# include "opentelemetry/exporters/geneva/common/socket_tools.h"
#include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace geneva
{
namespace metrics
{
    class UnixDomainSocketDataTransport: public DataTransport {
    public:
            UnixDomainSocketDataTransport(const ConnectionStringParser &connection_string);
            bool Connect() noexcept override;
            void Send(ByteVector &data) noexcept override;
            bool Disconnect() noexcept override;
    private:
        // Socket connection is re-established for every batch of events
        SocketTools::Socket socket_;
        SocketTools::SocketParams socketparams_{AF_INET, SOCK_STREAM, 0};
        nostd::unique_ptr<SocketTools::SocketAddr> addr_;
        bool connected_{false};

    };
}
}
}
OPENTELEMETRY_END_NAMESPACE