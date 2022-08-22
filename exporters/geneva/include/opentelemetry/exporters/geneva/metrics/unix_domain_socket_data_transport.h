// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/exporters/geneva/metrics/data_transport.h"
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
            bool Connect() noexcept override;
            void Send(ByteVector &data) noexcept override;
            bool Disconnect() noexcept override;
    };
}
}
}
OPENTELEMETRY_END_NAMESPACE