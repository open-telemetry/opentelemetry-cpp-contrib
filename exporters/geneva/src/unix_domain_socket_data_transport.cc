// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/exporters/geneva/metrics/unix_domain_socket_data_transport.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace geneva
{
namespace metrics
{

    UnixDomainSocketDataTransport::UnixDomainSocketDataTransport(const ConnectionStringParser &connection_string)
    {
        socketparams_ = {AF_UNIX, SOCK_STREAM, 0};
        addr_.reset(
            new SocketTools::SocketAddr(connection_string.c_str(), true));
    }

    bool Connect() noexcept
    {
        if (!connected_) {
             socket_ = SocketTools::Socket(socketparams_);
             connected_ = socket_.connect(*addr_);
             if (!connected_){
                //TBD - Log Error
                return false;
             }
        }
        return true;
    }

    bool Send(ByteVector &data) noexcept
    {
        int error_code = 0;
        if (connected_){
            socket_.getsockopt(SOL_SOCKET, SO_ERROR, error_code);
        }
        if (error_code != 0){
            connected_ = false;
        }

        // try to write
        size_t sent_size = socket_.writeall(data);
        if (data.size() == sent_size){
            Disconnect();
            return true;
        }
        else {
            //TBD - Log error
        }
        return false;
    }

    bool Disconnect() noexcept
    {
        if (connected_){
            connected_ = false;
            if (socket_.invalid()){
                socket_.close();
                return true;
            }
        }
        return false;
    }
}
}
}
OPENTELEMETRY_END_NAMESPACE