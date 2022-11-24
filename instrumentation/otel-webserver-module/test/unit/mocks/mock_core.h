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
#include "AgentCore.h"
#include "api/ApiUtils.h"
#include "api/RequestProcessingEngine.h"

class MockAgentCore : public appd::core::ICore
{
public:
	MOCK_METHOD(bool,  start,
		(std::shared_ptr<appd::core::TenantConfig> initConfig,
        std::shared_ptr<appd::core::SpanNamer> spanNamer,
        userAddedTenantMap& userAddedTenants),
        (override));

    MOCK_METHOD(void, stop, (), (override));

    MOCK_METHOD(appd::core::IRequestProcessingEngine*, getRequestProcessor,
    	(std::string& name), (override));

    MOCK_METHOD(void, addContext,
    	(const std::string& contextName,
        std::shared_ptr<appd::core::TenantConfig> newConfig),
        (override));

    MOCK_METHOD(std::shared_ptr<appd::core::IContext>,
        getWebServerContext, (std::string& name), (override));
};

class MockApiUtils : public appd::core::IApiUtils
{
public:
	MOCK_METHOD(OTEL_SDK_STATUS_CODE, init_boilerplate, (), (override));

    MOCK_METHOD(OTEL_SDK_STATUS_CODE, ReadFromPassedSettings,
    	(OTEL_SDK_ENV_RECORD* env,
        unsigned numberOfRecords,
        appd::core::TenantConfig& tenantConfig,
        appd::core::SpanNamer& spanNamer),  (override));
    /*MOCK_METHOD(OTEL_SDK_STATUS_CODE, ReadFromEnvinronment,
    	(appd::core::TenantConfig& tenantConfig), (override));*/
    MOCK_METHOD(OTEL_SDK_STATUS_CODE, ReadSettingsFromReader,
    	(appd::core::IEnvReader& reader, appd::core::TenantConfig& tenantConfig,
        appd::core::SpanNamer& spanNamer),  (override));
    /*MOCK_METHOD(OTEL_SDK_STATUS_CODE, ReadMandatoryFromReader,
    	(appd::core::IEnvReader& reader, const std::string& varName, unsigned short& result),
    	(override));*/
    MOCK_METHOD(OTEL_SDK_STATUS_CODE, ReadOptionalFromReader,
    	(appd::core::IEnvReader& reader, const std::string& varName, bool& result),
    	(override));
    MOCK_METHOD(OTEL_SDK_STATUS_CODE, ReadOptionalFromReader,
    	(appd::core::IEnvReader& reader, const std::string& varName, unsigned int& result),
    	(override));
};

class MockAgentKernel : public appd::core::IKernel
{
public:
    MOCK_METHOD(void, initKernel,
        (std::shared_ptr<appd::core::TenantConfig> config,
        std::shared_ptr<appd::core::SpanNamer> spanNamer), (override));

    MOCK_METHOD(appd::core::IRequestProcessingEngine*, getRequestProcessingEngine,
        (), (override));
};

class MockContext : public appd::core::IContext
{
public:
    MOCK_METHOD(void, initContext,
        (std::shared_ptr<appd::core::SpanNamer> spanNamer), (override));

    MOCK_METHOD(appd::core::IKernel*, getKernel, (), (const, override));

    MOCK_METHOD(std::shared_ptr<appd::core::TenantConfig>, getConfig, (), (override));
};
