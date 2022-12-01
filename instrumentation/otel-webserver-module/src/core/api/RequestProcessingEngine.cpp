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

#include "api/RequestProcessingEngine.h"
#include "AgentLogger.h"
#include "api/ApiUtils.h"
#include "api/Payload.h"
#include "sdkwrapper/SdkWrapper.h"
#include "sdkwrapper/IScopedSpan.h"
#include "sdkwrapper/SdkConstants.h"
#include <sys/types.h>
#include <unistd.h>
#include <boost/filesystem.hpp>
#include <boost/thread/locks.hpp>
#include <log4cxx/logger.h>
#include <unordered_map>
#include <sstream>


namespace otel {
namespace core {

using namespace sdkwrapper;

constexpr const char* http_request_header = "http.request.header.";
constexpr const char* http_response_header = "http.response.header.";

RequestProcessingEngine::RequestProcessingEngine()
    : mLogger(getLogger(std::string(LogContext::AGENT) + ".RequestProcessingEngine"))
{
}

void RequestProcessingEngine::init(std::shared_ptr<TenantConfig>& config,
    std::shared_ptr<SpanNamer> spanNamer) {
    m_sdkWrapper.reset(new sdkwrapper::SdkWrapper());
    m_sdkWrapper->Init(config);
    m_spanNamer = spanNamer;
}

OTEL_SDK_STATUS_CODE RequestProcessingEngine::startRequest(
    const std::string& wscontext,
    RequestPayload* payload,
    OTEL_SDK_HANDLE_REQ* reqHandle) {
    if (!reqHandle) {
        LOG4CXX_ERROR(mLogger, __FUNCTION__ << " " << OTEL_STATUS(handle_pointer_is_null));
        return OTEL_STATUS(handle_pointer_is_null);
    }

    if (!payload) {
        LOG4CXX_ERROR(mLogger, __FUNCTION__ << " " << OTEL_STATUS(payload_reflector_is_null));
        return OTEL_STATUS(payload_reflector_is_null);
    }

    std::string spanName = m_spanNamer->getSpanName(payload->get_uri());
    otel::core::sdkwrapper::OtelKeyValueMap keyValueMap;
    keyValueMap[kAttrRequestProtocol] = payload->get_request_protocol();
    keyValueMap[kAttrHTTPServerName] = payload->get_server_name();
    keyValueMap[kAttrHTTPMethod] = payload->get_http_request_method();
    keyValueMap[kAttrHTTPScheme] = payload->get_scheme();
    keyValueMap[kAttrNetHostName] = payload->get_host();
    keyValueMap[kAttrNETHostPort] = payload->get_port();
    keyValueMap[kAttrHTTPTarget] =payload->get_target();
    keyValueMap[kAttrHTTPFlavor] = payload->get_flavor();
    keyValueMap[kAttrHTTPClientIP] = payload->get_client_ip();

    auto& request_headers = payload->get_request_headers();
    for (auto itr = request_headers.begin(); itr != request_headers.end(); itr++) {
        std::string key = std::string(http_request_header) +
                std::string(itr->first);
        keyValueMap[key] = itr->second;
    }
    auto span = m_sdkWrapper->CreateSpan(spanName, sdkwrapper::SpanKind::SERVER, keyValueMap, payload->get_http_headers());

    LOG4CXX_TRACE(mLogger, "Span started for context: [" << wscontext
        <<"] SpanName: " << spanName << ", RequestProtocol: " << payload->get_request_protocol()
        <<" SpanId: " << span.get());
    RequestContext* requestContext = new RequestContext(span);
    requestContext->setContextName(wscontext);

    // Fill the requestHandle
    *reqHandle = (void*)requestContext;

    return OTEL_SUCCESS;
}

OTEL_SDK_STATUS_CODE RequestProcessingEngine::endRequest(
    OTEL_SDK_HANDLE_REQ reqHandle,
    const char* error,
    const ResponsePayload* payload) {

    if (!reqHandle) {
        LOG4CXX_ERROR(mLogger, "Invalid request, can't end request");
        return OTEL_STATUS(fail);
    }

    RequestContext* requestContext = (RequestContext*)reqHandle;
    auto rootSpan = requestContext->rootSpan();

    // End all the active interactions one by one if they aren't closed.
    // Even if we end parent span, child spans might remain active. We need
    // to ensure that all client/internal spans are closed (in case of error in request
    // they might not be explicitely closed)

    while (requestContext->hasActiveInteraction()) {
        auto interactionSpan = requestContext->lastActiveInteraction();
        LOG4CXX_TRACE(mLogger, "Ending Span with id: " << interactionSpan.get());
        interactionSpan->End();
        requestContext->removeInteraction();
    }

    // check for error and set attribute in the scopedSpan.
    if (error) {
        std::stringstream strValue;
        unsigned int errorValue;

        strValue << error;
        strValue >> errorValue;

        std::string errorStatus (kHttpErrorCode + error); // This is status message eg: HTTP ERROR CODE:403

        if (errorValue >= HTTP_ERROR_1XX &&   errorValue < HTTP_ERROR_4XX ) {
            rootSpan->SetStatus(StatusCode::Unset);
        }
        else if (errorValue >= HTTP_ERROR_4XX &&   errorValue < HTTP_ERROR_5XX ) {
            if (rootSpan->GetSpanKind() == SpanKind::SERVER)
                rootSpan->SetStatus(StatusCode::Unset);
            else
                rootSpan->SetStatus(StatusCode::Error, errorStatus);

        } else {
            rootSpan->SetStatus(StatusCode::Error, errorStatus);
        }

        LOG4CXX_TRACE(mLogger, "Setting status as error[" << errorStatus <<"] on root Span");

    } else {
        rootSpan->SetStatus(StatusCode::Ok);
    }
    if (payload != nullptr) {
        for (auto itr = payload->response_headers.begin();
            itr != payload->response_headers.end(); itr++) {
            std::string key = std::string(http_response_header) +
                std::string(itr->first);
            rootSpan->AddAttribute(key, itr->second);
        }

        rootSpan->AddAttribute(kAttrHTTPStatusCode, payload->status_code);
    }

    LOG4CXX_TRACE(mLogger, "Ending root span with id: " << rootSpan.get());
    rootSpan->End();
    delete requestContext;

    return OTEL_SUCCESS;
}

OTEL_SDK_STATUS_CODE RequestProcessingEngine::startInteraction(
    OTEL_SDK_HANDLE_REQ reqHandle,
    const InteractionPayload* payload,
    std::unordered_map<std::string, std::string>& propagationHeaders) {

    if (!reqHandle) {
        LOG4CXX_ERROR(mLogger, __FUNCTION__ << " " << OTEL_STATUS(handle_pointer_is_null));
        return OTEL_STATUS(handle_pointer_is_null);
    }

    if (!payload) {
        LOG4CXX_ERROR(mLogger, __FUNCTION__ << " " << OTEL_STATUS(payload_reflector_is_null));
        return OTEL_STATUS(payload_reflector_is_null);
    }

    RequestContext* requestContext = (RequestContext*)reqHandle;

    // TODO : Add internal spans for virtual hosts post MVP

    // Create client span for this interaction. And set it in RequestContext object.
    otel::core::sdkwrapper::OtelKeyValueMap keyValueMap;

    // TODO : confirm and update name later
    std::string spanName = payload->moduleName + "_" + payload->phaseName;
    keyValueMap["interactionType"] = "EXIT_CALL";
    auto interactionSpan = m_sdkWrapper->CreateSpan(spanName, SpanKind::CLIENT, keyValueMap);
    LOG4CXX_TRACE(mLogger, "Client Span started with SpanName: " << spanName
        << " Span Id: " << interactionSpan.get());
    m_sdkWrapper->PopulatePropagationHeaders(propagationHeaders);

    // Add the interaction to the request context.
    requestContext->addInteraction(interactionSpan);
    return OTEL_SUCCESS;
}

OTEL_SDK_API OTEL_SDK_STATUS_CODE RequestProcessingEngine::endInteraction(
    OTEL_SDK_HANDLE_REQ reqHandle,
    bool ignoreBackend,
    EndInteractionPayload *payload) {

    if (!reqHandle) {
        LOG4CXX_ERROR(mLogger, __FUNCTION__ << " " << OTEL_STATUS(handle_pointer_is_null));
        return OTEL_STATUS(handle_pointer_is_null);
    }

    if (!payload) {
        LOG4CXX_ERROR(mLogger, __FUNCTION__ << " " << OTEL_STATUS(payload_reflector_is_null));
        return OTEL_STATUS(payload_reflector_is_null);
    }

    // TODO : incorporate ignore backend logic here.

    RequestContext* rContext = (RequestContext*)reqHandle;

    if (!rContext->hasActiveInteraction()) {
        // error : requestContext has no corresponding interaction.
        LOG4CXX_TRACE(mLogger, __FUNCTION__ << " " << OTEL_STATUS(invalid_context));
        return OTEL_STATUS(invalid_context);
    }

    auto interactionSpan = rContext->lastActiveInteraction();

    // If errorCode is 0 or errMsg is empty, there is no error.
    bool isError = payload->errorCode != 0 && !payload->errorMsg.empty();
    if (isError) {
        if (payload->errorCode >= HTTP_ERROR_1XX &&   payload->errorCode < HTTP_ERROR_4XX ) {
            interactionSpan->SetStatus(StatusCode::Unset);
        }
        else if (payload->errorCode >= HTTP_ERROR_4XX &&   payload->errorCode < HTTP_ERROR_5XX ) {
            if (interactionSpan->GetSpanKind() == SpanKind::SERVER)
                interactionSpan->SetStatus(StatusCode::Unset);
            else
                interactionSpan->SetStatus(StatusCode::Error, payload->errorMsg);

        } else {
            interactionSpan->SetStatus(StatusCode::Error, payload->errorMsg);
        }
        interactionSpan->AddAttribute("error_code", payload->errorCode);
        LOG4CXX_TRACE(mLogger, "Span updated with error Code: " << payload->errorCode);
    } else {
        interactionSpan->SetStatus(StatusCode::Ok);
    }

    if (!payload->backendName.empty()) {
        // TODO : update span name when api is added.
        interactionSpan->AddAttribute("backend_name", payload->backendName);
        interactionSpan->AddAttribute("backend_type", payload->backendType);
        LOG4CXX_TRACE(mLogger, "Span updated with BackendName: " << payload->backendName
            << " BackendType: " << payload->backendType);
    }

    LOG4CXX_TRACE(mLogger, "Ending Span with id: " << interactionSpan.get());
    interactionSpan->End();
    rContext->removeInteraction();
    return OTEL_SUCCESS;
}

} // core
} // otel
