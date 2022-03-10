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
#include <gmock/gmock.h>
#include "api/RequestProcessingEngine.h"
#include "api/Payload.h"
#include "api/TenantConfig.h"

class MockRequestProcessingEngine : public appd::core::IRequestProcessingEngine
{
public:

    MOCK_METHOD(void, init, (std::shared_ptr<appd::core::TenantConfig>& config,
        std::shared_ptr<appd::core::SpanNamer> spanNamer), (override));

    MOCK_METHOD(APPD_SDK_STATUS_CODE, startRequest, (
        const std::string& wscontext,
        appd::core::RequestPayload* payload,
        APPD_SDK_HANDLE_REQ* reqHandle), (override));

    MOCK_METHOD(APPD_SDK_STATUS_CODE, endRequest, (
        APPD_SDK_HANDLE_REQ reqHandle,
        const char* error), (override));

    APPD_SDK_STATUS_CODE startInteraction(
        APPD_SDK_HANDLE_REQ reqHandle,
        const appd::core::InteractionPayload* payload,
        std::unordered_map<std::string, std::string>& propagationHeaders) override {
    	return startInteractionImpl(reqHandle, payload);
    };

    MOCK_METHOD(APPD_SDK_STATUS_CODE, startInteractionImpl, (
        APPD_SDK_HANDLE_REQ reqHandle,
        const appd::core::InteractionPayload* payload));

    MOCK_METHOD(APPD_SDK_STATUS_CODE, endInteraction, (
        APPD_SDK_HANDLE_REQ reqHandle,
        bool ignoreBackend,
        appd::core::EndInteractionPayload *payload), (override));
};
