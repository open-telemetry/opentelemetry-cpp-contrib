/*
* Copyright 2021 AppDynamics LLC. 
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

#ifndef APPD_WS_AGENT_H
#define APPD_WS_AGENT_H

#include "api/AppdynamicsSdk.h"
#include <string>
#include <memory>
#include <unordered_map>
#include "api/Interface.h"
#include "api/TenantConfig.h"
#include "api/Payload.h"
#include <mutex>

namespace appd {
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

    APPD_SDK_STATUS_CODE init(
        APPD_SDK_ENV_RECORD* env,
        unsigned numberOfRecords);

    APPD_SDK_STATUS_CODE term();

    APPD_SDK_STATUS_CODE startRequest(
        const char* wscontext,
        RequestPayload* payload,
        APPD_SDK_HANDLE_REQ* reqHandle);

    APPD_SDK_STATUS_CODE endRequest(
        APPD_SDK_HANDLE_REQ reqHandle,
        const char* error,
        const ResponsePayload* payload = nullptr );

    APPD_SDK_STATUS_CODE startInteraction(
        APPD_SDK_HANDLE_REQ reqHandle,
        const InteractionPayload* payload,
        std::unordered_map<std::string, std::string>& propagationHeaders);

    APPD_SDK_STATUS_CODE endInteraction(
        APPD_SDK_HANDLE_REQ reqHandle,
        bool ignoreBackend,
        EndInteractionPayload *payload);

    /**
    * Add webserver context to AppDynamics configuration for multi-tenancy.
    *
    * @param contextName
    *     A unique identifier to refer to the context by
    * @param contextConfig
    *     AppDynamics context configuration object
    */
    APPD_SDK_API int addWSContextToCore(
        const char* wscontext,
        WSContextConfig* contextConfig);

private:

    unsigned long long initPid;
    std::mutex initMutex;

    APPD_SDK_STATUS_CODE checkPID();
    void initialisePid();
    APPD_SDK_STATUS_CODE validateAndInitialise();
    APPD_SDK_STATUS_CODE readConfig(APPD_SDK_ENV_RECORD* env,
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
