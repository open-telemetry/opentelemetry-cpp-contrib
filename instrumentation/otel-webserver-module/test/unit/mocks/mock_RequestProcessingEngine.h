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
#include <gmock/gmock.h>
#include "api/RequestProcessingEngine.h"
#include "api/Payload.h"
#include "api/TenantConfig.h"

class MockRequestProcessingEngine : public otel::core::IRequestProcessingEngine
{
public:

    MOCK_METHOD(void, init, (std::shared_ptr<otel::core::TenantConfig>& config,
        std::shared_ptr<otel::core::SpanNamer> spanNamer), (override));

    MOCK_METHOD(OTEL_SDK_STATUS_CODE, startRequest, (
        const std::string& wscontext,
        otel::core::RequestPayload* payload,
        OTEL_SDK_HANDLE_REQ* reqHandle), (override));

    MOCK_METHOD(OTEL_SDK_STATUS_CODE, endRequest, (
        OTEL_SDK_HANDLE_REQ reqHandle,
        const char* error,
        const otel::core::ResponsePayload* payload), (override));

    OTEL_SDK_STATUS_CODE startInteraction(
        OTEL_SDK_HANDLE_REQ reqHandle,
        const otel::core::InteractionPayload* payload,
        std::unordered_map<std::string, std::string>& propagationHeaders) override {
    	return startInteractionImpl(reqHandle, payload);
    };

    MOCK_METHOD(OTEL_SDK_STATUS_CODE, startInteractionImpl, (
        OTEL_SDK_HANDLE_REQ reqHandle,
        const otel::core::InteractionPayload* payload));

    MOCK_METHOD(OTEL_SDK_STATUS_CODE, endInteraction, (
        OTEL_SDK_HANDLE_REQ reqHandle,
        bool ignoreBackend,
        otel::core::EndInteractionPayload *payload), (override));
};
