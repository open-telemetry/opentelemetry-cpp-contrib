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

#ifndef REQUEST_PROCESSOR_ENGINE_H
#define REQUEST_PROCESSOR_ENGINE_H

#include "api/OpentelemetrySdk.h"
#include "api/SpanNamer.h"
#include "RequestContext.h"
#include "AgentLogger.h"
#include <unordered_map>


namespace otel {
namespace core {

class TenantConfig;
class RequestPayload;
class ResponsePayload;
class InteractionPayload;
class EndInteractionPayload;

namespace sdkwrapper {

class ISdkWrapper;

} //sdkwrapper

// Interface for RequestProcessingEngine
class IRequestProcessingEngine {
public:
    virtual ~IRequestProcessingEngine() {}
    virtual void init(std::shared_ptr<TenantConfig>& config,
        std::shared_ptr<SpanNamer> spanNamer) = 0;
    virtual OTEL_SDK_STATUS_CODE startRequest(
        const std::string& wscontext,
        RequestPayload* payload,
        OTEL_SDK_HANDLE_REQ* reqHandle) = 0;
    virtual OTEL_SDK_STATUS_CODE endRequest(
        OTEL_SDK_HANDLE_REQ reqHandle,
        const char* error,
        const ResponsePayload* payload = nullptr) = 0;
    virtual OTEL_SDK_STATUS_CODE startInteraction(
        OTEL_SDK_HANDLE_REQ reqHandle,
        const InteractionPayload* payload,
        std::unordered_map<std::string, std::string>& propagationHeaders) = 0;
    virtual OTEL_SDK_STATUS_CODE endInteraction(
        OTEL_SDK_HANDLE_REQ reqHandle,
        bool ignoreBackend,
        EndInteractionPayload *payload) = 0;
};

// Tracks request flow in agent using sdkWrapper
class RequestProcessingEngine : public IRequestProcessingEngine {
public:
    RequestProcessingEngine();
    ~RequestProcessingEngine() = default;

    void init(std::shared_ptr<TenantConfig>& config,
        std::shared_ptr<SpanNamer> spanNamer) override;
    OTEL_SDK_STATUS_CODE startRequest(
        const std::string& wscontext,
        RequestPayload* payload,
        OTEL_SDK_HANDLE_REQ* reqHandle) override;
    OTEL_SDK_STATUS_CODE endRequest(
        OTEL_SDK_HANDLE_REQ reqHandle,
        const char* error,
        const ResponsePayload* payload = nullptr) override;
    OTEL_SDK_STATUS_CODE startInteraction(
        OTEL_SDK_HANDLE_REQ reqHandle,
        const InteractionPayload* payload,
        std::unordered_map<std::string, std::string>& propagationHeaders) override;
    OTEL_SDK_API OTEL_SDK_STATUS_CODE endInteraction(
        OTEL_SDK_HANDLE_REQ reqHandle,
        bool ignoreBackend,
        EndInteractionPayload *payload) override;

protected:
    std::shared_ptr<otel::core::sdkwrapper::ISdkWrapper> m_sdkWrapper;
    std::shared_ptr<otel::core::SpanNamer> m_spanNamer;
private:
    AgentLogger mLogger;
};

} // core
} // otel

#endif
