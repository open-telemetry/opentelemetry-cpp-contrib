// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/geneva/metrics/unix_domain_socket_data_transport.h"
#include "opentelemetry/exporters/geneva/metrics/uuid.h"

#include "opentelemetry/exporters/geneva/metrics/macros.h"

ETWDataTransport::ETWDataTransport(const std::string &etw_provider)
{
    UUID guid = (etw_provider.rfind("{", 0) == 0) 
            ? UUID(etw_provider.c_str()) /// It's a ProviderGUID 
            : GetProviderGuid(etw_provider.c_str());  // It's a ProviderName
    
}