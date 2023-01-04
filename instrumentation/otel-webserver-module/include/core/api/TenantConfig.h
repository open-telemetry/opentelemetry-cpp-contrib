/*
* Copyright 2022, OpenTelemetry Authors. 
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#pragma once

#include <string>
#include <iostream>

namespace otel {
namespace core {

class TenantConfig
{
public:
    TenantConfig(){}

    bool isSameNamespaceNameId(const TenantConfig& cfg)
    {
        return  serviceNamespace == cfg.getServiceNamespace() &&
                serviceName == cfg.getServiceName() &&
                serviceInstanceId == cfg.getServiceInstanceId();
    }

    // This function is used to copy all the attribute from the initial core init config to the user added
    // tenants. Since we only allow the user to specify service namespace/name/id, we must get all the other attributes
    // from the core initial config
    void copyAllExceptNamespaceNameId(const TenantConfig& copyfrom);

    const std::string& getServiceNamespace() const {return serviceNamespace;}
    const std::string& getServiceName() const {return serviceName;}
    const std::string& getServiceInstanceId() const {return serviceInstanceId;}
    const std::string& getOtelLibraryName() const {return otelLibraryName;}
    const std::string& getOtelLibraryVersion() const {return otelLibraryVersion;}
    const std::string& getOtelExporterType() const {return otelExporterType;}
    const std::string& getOtelExporterEndpoint() const {return otelExporterEndpoint;}
    const std::string& getOtelExporterOtlpHeaders() const {return otelExporterOtlpHeaders;}
    const std::string& getOtelProcessorType() const {return otelProcessorType;}
    const unsigned getOtelMaxQueueSize() const {return otelMaxQueueSize;}
    const unsigned getOtelScheduledDelayMillis() const {return otelScheduledDelayMillis;}
    const unsigned getOtelExportTimeoutMillis() const {return otelExportTimeoutMillis;}
    const unsigned getOtelMaxExportBatchSize() const {return otelMaxExportBatchSize;}
    const std::string& getOtelSamplerType() const {return otelSamplerType;}
    const bool getOtelSslEnabled() const { return otelSslEnabled; }
    const std::string& getOtelSslCertPath() const { return otelSslCertPath; }

    void setOtelLibraryName(const std::string& name) { this->otelLibraryName = name; }
    void setOtelLibraryVersion(const std::string& version) { this->otelLibraryVersion = version; }
    void setServiceNamespace(const std::string& serviceNamespace) { this->serviceNamespace = serviceNamespace; }
    void setServiceName(const std::string& serviceName) { this->serviceName = serviceName; }
    void setServiceInstanceId(const std::string& serviceInstanceId) { this->serviceInstanceId = serviceInstanceId; }
    void setOtelExporterType(const std::string& otelExporterType) { this->otelExporterType = otelExporterType; }
    void setOtelExporterEndpoint(const std::string& otelExporterEndpoint) { this->otelExporterEndpoint = otelExporterEndpoint; }
    void setOtelExporterOtlpHeaders(const std::string& otelExporterOtlpHeaders) { this->otelExporterOtlpHeaders = otelExporterOtlpHeaders; }
    void setOtelProcessorType(const std::string& otelProcessorType) { this->otelProcessorType = otelProcessorType; }
    void setOtelMaxQueueSize(const unsigned int otelMaxQueueSize) { this->otelMaxQueueSize = otelMaxQueueSize; }
    void setOtelScheduledDelayMillis(const unsigned int otelScheduledDelayMillis) { this->otelScheduledDelayMillis = otelScheduledDelayMillis; }
    void setOtelExportTimeoutMillis(const unsigned int otelExportTimeoutMillis) { this->otelExportTimeoutMillis = otelExportTimeoutMillis; }
    void setOtelMaxExportBatchSize(const unsigned int otelMaxExportBatchSize) {this->otelMaxExportBatchSize = otelMaxExportBatchSize; }
    void setOtelSamplerType(const std::string& otelSamplerType) { this->otelSamplerType = otelSamplerType; }
    void setOtelSslEnabled(const bool& otelSslEnabled) { this->otelSslEnabled = otelSslEnabled; }
    void setOtelSslCertPath(const std::string& otelSslCertPath) { this->otelSslCertPath = otelSslCertPath; }

private:
    std::string serviceNamespace;
    std::string serviceName;
    std::string serviceInstanceId;

    std::string otelLibraryName;
    std::string otelLibraryVersion;

    std::string otelExporterType;
    std::string otelExporterEndpoint;
    std::string otelExporterOtlpHeaders;
    bool otelSslEnabled;
    std::string otelSslCertPath;

    std::string otelProcessorType;
    std::string otelSamplerType;

    unsigned otelMaxQueueSize;
    unsigned otelScheduledDelayMillis;
    unsigned otelExportTimeoutMillis;
    unsigned otelMaxExportBatchSize;

    //Span Limits(AttributeCountLimit, EventCountLimit, LinkCountLimit, AttributePerEventCountLimit, AttributePerLinkCountLimit
    //: configuration options have not been added for these; as of now they will have their default vaules
};

inline std::ostream& operator<< (std::ostream &os, const otel::core::TenantConfig &config)
{
    os  << "\n ServiceNamespace:                " << config.getServiceNamespace()
        << "\n ServiceName:                     " << config.getServiceName()
        << "\n ServiceInstanceId                " << config.getServiceInstanceId()
        << "\n OtelLibraryName                  " << config.getOtelLibraryName()
        << "\n OtelLibraryVersion               " << config.getOtelLibraryVersion()
        << "\n OtelExporterType                 " << config.getOtelExporterType()
        << "\n OtelExporterEndpoint             " << config.getOtelExporterEndpoint()
        << "\n OtelProcessorType                " << config.getOtelProcessorType()
        << "\n OtelSamplerType                  " << config.getOtelSamplerType()
        << "\n OtelSslEnabled                   " << config.getOtelSslEnabled()
        << "\n OtelSslCertPath                  " << config.getOtelSslCertPath()
        << "\n OtelExportOtlpHeaders            " << config.getOtelExporterOtlpHeaders()
        << "";
    return os;
}

} // core
} // otel


