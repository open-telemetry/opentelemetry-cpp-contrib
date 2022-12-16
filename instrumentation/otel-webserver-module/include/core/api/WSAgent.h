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

#ifndef OTEL_WS_AGENT_H
#define OTEL_WS_AGENT_H

#include "api/OpentelemetrySdk.h"
#include <string>
#include <memory>
#include <unordered_map>
#include "api/Interface.h"
#include "api/TenantConfig.h"
#include "api/Payload.h"
#include <mutex>

namespace otel {
namespace core {

/**
 * Configuration an webserver context (tenant) for the Core.
 */
struct WSContextConfig
{
    std::string serviceNamespace;
    std::string serviceName;
    std::string serviceInstanceId;
};

class WSAgent
{
public:
    WSAgent();
    virtual ~WSAgent() = default;

    virtual void initDependency();

    bool isInitialised();

    OTEL_SDK_STATUS_CODE init(
        OTEL_SDK_ENV_RECORD* env,
        unsigned numberOfRecords);

    OTEL_SDK_STATUS_CODE term();

    OTEL_SDK_STATUS_CODE startRequest(
        const char* wscontext,
        RequestPayload* payload,
        OTEL_SDK_HANDLE_REQ* reqHandle);

    OTEL_SDK_STATUS_CODE endRequest(
        OTEL_SDK_HANDLE_REQ reqHandle,
        const char* error,
        const ResponsePayload* payload = nullptr );

    OTEL_SDK_STATUS_CODE startInteraction(
        OTEL_SDK_HANDLE_REQ reqHandle,
        const InteractionPayload* payload,
        std::unordered_map<std::string, std::string>& propagationHeaders);

    OTEL_SDK_STATUS_CODE endInteraction(
        OTEL_SDK_HANDLE_REQ reqHandle,
        bool ignoreBackend,
        EndInteractionPayload *payload);

    /**
    * Add webserver context to OpenTelemetry configuration for multi-tenancy.
    *
    * @param contextName
    *     A unique identifier to refer to the context by
    * @param contextConfig
    *     OpenTelemetry context configuration object
    */
    OTEL_SDK_API int addWSContextToCore(
        const char* wscontext,
        WSContextConfig* contextConfig);

private:

    unsigned long long initPid;
    std::mutex initMutex;

    OTEL_SDK_STATUS_CODE checkPID();
    void initialisePid();
    OTEL_SDK_STATUS_CODE validateAndInitialise();
    OTEL_SDK_STATUS_CODE readConfig(OTEL_SDK_ENV_RECORD* env,
        unsigned numberOfRecords, std::shared_ptr<TenantConfig> tenantConfig,
        std::shared_ptr<SpanNamer> spanNamer);

protected:

    std::unique_ptr<ICore> mAgentCore;
    std::unique_ptr<IApiUtils> mApiUtils;
    std::unordered_map<std::string,
        std::shared_ptr<TenantConfig>> mUserAddedTenant;
};

}
}

#endif
