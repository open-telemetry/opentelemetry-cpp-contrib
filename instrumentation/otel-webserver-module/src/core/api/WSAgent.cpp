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

#include "api/WSAgent.h"
#include "api/RequestProcessingEngine.h"
#include "api/OpentelemetrySdk.h"
#include "api/ApiUtils.h"
#include "RequestContext.h"
#include <sstream>
#include "AgentCore.h"
#include "api/SpanNamer.h"

namespace otel {
namespace core {

inline void apiFuncTraceError(const char* funcName, OTEL_SDK_STATUS_CODE ret)
{
    std::ostringstream errMsg;
    errMsg << "Error: " << funcName << ": Error Code: " << ret;
    if(ApiUtils::apiUserLogger)
    {
        LOG4CXX_ERROR(ApiUtils::apiUserLogger, errMsg.str());
    }
    else
    {
        std::cerr << errMsg.str() << std::endl;
    }
}

WSAgent::WSAgent() : initPid(0)
{}

OTEL_SDK_STATUS_CODE
WSAgent::init(OTEL_SDK_ENV_RECORD* env, unsigned numberOfRecords)
{
    std::lock_guard<std::mutex> lock(initMutex);
    OTEL_SDK_STATUS_CODE res = validateAndInitialise();
    if (OTEL_ISFAIL(res))
    {
        return res;
    }

    res = mApiUtils->init_boilerplate();
    if(OTEL_ISFAIL(res))
    {
        apiFuncTraceError(BOOST_CURRENT_FUNCTION, res);
        return(res);
    }

    // Read all Config passed in env or from environment
    std::shared_ptr<TenantConfig> tenantConfig(std::make_shared<TenantConfig>());
    std::shared_ptr<SpanNamer> spanNamer(std::make_shared<SpanNamer>());
    res = readConfig(env, numberOfRecords, tenantConfig, spanNamer);
    if(OTEL_ISFAIL(res))
    {
        apiFuncTraceError(BOOST_CURRENT_FUNCTION, res);
        return(res);
    }

    // Start the Agent Core.
    if (!mAgentCore->start(tenantConfig, spanNamer, mUserAddedTenant))
    {
        apiFuncTraceError(BOOST_CURRENT_FUNCTION, OTEL_STATUS(agent_failed_to_start));
        return(OTEL_STATUS(agent_failed_to_start));
    }

    initialisePid();

    return OTEL_SUCCESS;
}

OTEL_SDK_STATUS_CODE
WSAgent::term()
{
    if (mAgentCore)
    {
        mAgentCore->stop();
    }
}

OTEL_SDK_STATUS_CODE
WSAgent::startRequest(
    const char* wscontext,
    RequestPayload* requestPayload,
    OTEL_SDK_HANDLE_REQ* reqHandle)
{
    std::string context {wscontext};
    auto *engine = mAgentCore->getRequestProcessor(context);
    if (nullptr != engine)
    {
        return engine->startRequest(context, requestPayload, reqHandle);
    }

    return OTEL_STATUS(fail);
}

OTEL_SDK_STATUS_CODE
WSAgent::endRequest(
    OTEL_SDK_HANDLE_REQ reqHandle,
    const char* error, const ResponsePayload* payload)
{
    // TODO: How to get the context here?
    // one solution is get it from reqHandle.
    // RequestProcessor would put the context
    // in reqHandle during start Request phase.
    RequestContext *ctx = static_cast<RequestContext*>(reqHandle);
    auto context = ctx->getContextName();
    auto *engine = mAgentCore->getRequestProcessor(context);
    if (nullptr != engine)
    {
        return engine->endRequest(reqHandle, error, payload);
    }
    return OTEL_STATUS(fail);
}

OTEL_SDK_STATUS_CODE
WSAgent::startInteraction(
    OTEL_SDK_HANDLE_REQ reqHandle,
    const InteractionPayload* payload,
    std::unordered_map<std::string, std::string>& propagationHeaders)
{
    RequestContext *ctx = static_cast<RequestContext*>(reqHandle);
    auto context = ctx->getContextName();
    auto *engine = mAgentCore->getRequestProcessor(context);
    if (nullptr != engine)
    {
        return engine->startInteraction(reqHandle, payload, propagationHeaders);
    }
    return OTEL_STATUS(fail);
}

OTEL_SDK_STATUS_CODE
WSAgent::endInteraction(
    OTEL_SDK_HANDLE_REQ reqHandle,
    bool ignoreBackend,
    EndInteractionPayload *payload)
{
    RequestContext *ctx = static_cast<RequestContext*>(reqHandle);
    auto context = ctx->getContextName();
    auto *engine = mAgentCore->getRequestProcessor(context);
    if (nullptr != engine)
    {
        return engine->endInteraction(reqHandle, ignoreBackend, payload);
    }
    return OTEL_STATUS(fail);
}

int
WSAgent::addWSContextToCore(
    const char* wscontext,
    WSContextConfig* contextConfig)
{
    std::string contextName {wscontext};
    if (contextName.empty() || contextName == COREINIT_CONTEXT)
    {
        apiFuncTraceError("Invalid context name", OTEL_STATUS(cannot_add_ws_context_to_core));
        return -1;
    }

    if (!contextConfig)
    {
        apiFuncTraceError("Invalid context config", OTEL_STATUS(cannot_add_ws_context_to_core));
        return -1;
    }
    std::string serviceNamespace = contextConfig->serviceNamespace;
    std::string serviceName = contextConfig->serviceName;
    std::string serviceInstanceId = contextConfig->serviceInstanceId;

    if (serviceNamespace.empty())
    {
        apiFuncTraceError("Invalid serviceNamespace", OTEL_STATUS(cannot_add_ws_context_to_core));
        return -1;
    }

    if (serviceName.empty())
    {
        apiFuncTraceError("Invalid serviceName", OTEL_STATUS(cannot_add_ws_context_to_core));
        return -1;
    }

    if (serviceInstanceId.empty())
    {
        apiFuncTraceError("Invalid serviceInstanceId", OTEL_STATUS(cannot_add_ws_context_to_core));
        return -1;
    }

    std::shared_ptr<TenantConfig> tenant = std::make_shared<TenantConfig>();
    tenant->setServiceNamespace(serviceNamespace);
    tenant->setServiceName(serviceName);
    tenant->setServiceInstanceId(serviceInstanceId);

    {
        // Call to isinitialised should also be protected,
        // as initpid is accessed.
        std::lock_guard<std::mutex> lock(initMutex);
        if (isInitialised())
        {
            auto existingContext = mAgentCore->getWebServerContext(contextName);
            if (existingContext) // context already exists
            {
                if (!existingContext->getConfig()->isSameNamespaceNameId(*tenant))
                {
                    // New context has different service namepsace/name/id. Issue warning.
                    // TODO: Need to initialize logging correctly for testing.
                    std::cerr << "Warning: Context " << contextName << " already exists." << std::endl;
                }

                return 0;
            }
            mAgentCore->addContext(contextName, tenant);

            std::cerr << "Added context: " << contextName << " serviceNamespace: " << serviceNamespace
                << " serviceName: " << serviceName << " serviceInstanceId: " << serviceInstanceId << std::endl;
        }
        else
        {
            auto it = mUserAddedTenant.find(contextName);
            if (it != mUserAddedTenant.end()) // context already exists
            {
                if (!it->second->isSameNamespaceNameId(*tenant))
                {
                    // New context has different service namespace/name/id. Issue warning. Must use std::cerr
                    // because agent logging is not initialized yet
                    std::cerr << "Warning: " << BOOST_CURRENT_FUNCTION << ": Context "
                        << contextName << " already exists." << std::endl;
                }

                return 0;
            }

            mUserAddedTenant[contextName] = tenant;
        }
    }
    return 0;
}

void
WSAgent::initDependency() {
    std::lock_guard<std::mutex> lock(initMutex);
    if (mAgentCore.get() == nullptr)
    {
        mAgentCore.reset(new AgentCore());
    }
    if (mApiUtils.get() == nullptr)
    {
        mApiUtils.reset(new ApiUtils());
    }
}

OTEL_SDK_STATUS_CODE
WSAgent::checkPID()
{
    if (!initPid)
    {
        return otel_sdk_status_uninitialized;
    }
    else if (initPid != getpid())
    {
        return otel_sdk_status_wrong_process_id;
    }
    else
    {
        return otel_sdk_status_success;
    }
}

void
WSAgent::initialisePid()
{
    initPid = getpid();
}

OTEL_SDK_STATUS_CODE
WSAgent::validateAndInitialise()
{
    OTEL_SDK_STATUS_CODE res = checkPID();
    if (res == otel_sdk_status_uninitialized)
    {
        // this is expected and means it is a valid sdk_init call.
        res = otel_sdk_status_success;
    }
    else if (res == otel_sdk_status_success)
    {
        // this means sdk_init was already called
        apiFuncTraceError(BOOST_CURRENT_FUNCTION, otel_sdk_status_already_initialized);
        return(otel_sdk_status_already_initialized);
    }
    else if(OTEL_ISFAIL(res))
    {
        apiFuncTraceError(BOOST_CURRENT_FUNCTION, res);
        return(res);
    }

    return res;
}

OTEL_SDK_STATUS_CODE
WSAgent::readConfig(
    OTEL_SDK_ENV_RECORD* env,
    unsigned numberOfRecords,
    std::shared_ptr<TenantConfig> tenantConfig,
    std::shared_ptr<SpanNamer> spanNamer)
{
    // Read all Config passed in env or from environment
    OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;
    /*if(!env || numberOfRecords == 0)
    {
        res = mApiUtils->ReadFromEnvinronment(*tenantConfig);
    }
    else*/
    {
        res = mApiUtils->ReadFromPassedSettings(env, numberOfRecords, *tenantConfig, *spanNamer);
    }
    return res;
}

bool
WSAgent::isInitialised()
{
    // TODO: Do we need to take lock here ?
    return initPid;
}

}
}
