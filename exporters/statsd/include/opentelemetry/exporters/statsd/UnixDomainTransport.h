// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "statsd_common.h"
#include "opentelemetry/version.h"

#include <string>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/un.h>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace statsd {
namespace metrics {


class UnixDomainClient 
{

UnixDomainClient(std::string path): socket_path_(path){}
bool Initialize()
{
    server_socket_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket_fd_ == -1) {
        // log error
        return false;
    }
    sockaddr_un remote;
    remote.sun_family = AF_UNIX;
    strncpy_s(remote.sun_path, sizeof(remote.sun_path), socket_path_.c_str(),
                sizeof(remote.sun_path));
    if (connect(server_socket_fd_, (sockaddr*)&remote, strlen(remote.sun_path) + sizeof(remote.sun_family)) == -1) {
        //connect failed
        ::close(server_socket_fd_);
        server_socket_fd_ = 0;
        return false;
    }
    //success
    return true;
}

bool Send(const char* data, uint32_t size);

private:
std::string socket_path_;
uint16_t server_socket_fd_ = 0;

}
}
}
}
OPENTELEMETRY_END_NAMESPACE
