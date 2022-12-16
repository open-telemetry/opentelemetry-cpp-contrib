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

#include <unordered_map>
#include <api/OpentelemetrySdk.h>
#include <api/TenantConfig.h>
#include <api/SpanNamer.h>

namespace otel {
namespace core {

class IRequestProcessingEngine;

class IEnvReader
{
public:
    virtual OTEL_SDK_STATUS_CODE ReadMandatory(
    	const std::string& varName, std::string& result) = 0;
    virtual OTEL_SDK_STATUS_CODE ReadOptional(
    	const std::string& varName, std::string& result) = 0;
};

class IApiUtils
{
public:
    virtual ~IApiUtils() = default;

    virtual OTEL_SDK_STATUS_CODE init_boilerplate() = 0; // initializes agentLogging

    virtual OTEL_SDK_STATUS_CODE ReadFromPassedSettings(
            OTEL_SDK_ENV_RECORD* env,
            unsigned numberOfRecords,
            TenantConfig& tenantConfig,
            SpanNamer& spanNamer) = 0;
protected:
    //virtual OTEL_SDK_STATUS_CODE ReadFromEnvinronment(TenantConfig&) = 0;
    virtual OTEL_SDK_STATUS_CODE ReadSettingsFromReader(
        IEnvReader& reader, TenantConfig&, SpanNamer&) = 0;
    /*virtual OTEL_SDK_STATUS_CODE ReadMandatoryFromReader(
        IEnvReader& reader, const std::string& varName, unsigned short& result) = 0;*/
    virtual OTEL_SDK_STATUS_CODE ReadOptionalFromReader(
        IEnvReader& reader, const std::string& varName, bool& result) = 0;
    virtual OTEL_SDK_STATUS_CODE ReadOptionalFromReader(
        IEnvReader& reader, const std::string& varName, unsigned int& result) = 0;
};

class IKernel
{
public:
    virtual ~IKernel() = default;

    virtual void initKernel(std::shared_ptr<otel::core::TenantConfig> config,
        std::shared_ptr<otel::core::SpanNamer> spanNamer) = 0;

    virtual IRequestProcessingEngine* getRequestProcessingEngine() = 0;
};

class IContext
{
public:
    virtual ~IContext() = default;

    virtual void initContext(std::shared_ptr<otel::core::SpanNamer> spanNamer) = 0;

    virtual IKernel* getKernel() const = 0;

    virtual std::shared_ptr<otel::core::TenantConfig> getConfig() = 0;
};

class ICore
{
public:
    using userAddedTenantMap =
    	std::unordered_map<std::string, std::shared_ptr<TenantConfig>>;

    virtual ~ICore() = default;

    virtual bool start(
        std::shared_ptr<otel::core::TenantConfig> initConfig,
        std::shared_ptr<otel::core::SpanNamer> spanNamer,
        userAddedTenantMap& userAddedTenants) = 0;

    virtual void stop() = 0;

    virtual IRequestProcessingEngine* getRequestProcessor(std::string& name) = 0;

    virtual void addContext(
        const std::string& contextName,
        std::shared_ptr<TenantConfig> newConfig) = 0;

    virtual std::shared_ptr<IContext>
        getWebServerContext(std::string& name) = 0;
};

}
}
