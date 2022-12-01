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

#ifndef __AGENTCORE_H
#define __AGENTCORE_H

#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>
#include "api/Interface.h"
#include "AgentLogger.h"
#include "api/RequestProcessingEngine.h"
#include "api/TenantConfig.h"

// Contexts did not exist in the beginning. Users put in the serviceNamespace/serviceName/serviceInstanceId
//  in the init config structure.
#define COREINIT_CONTEXT "OTEL_COREINIT_CONTEXT"

namespace otel {
namespace core {

class AgentKernel : public IKernel
{
public:
    AgentKernel();
    ~AgentKernel() = default;

    void initKernel(std::shared_ptr<otel::core::TenantConfig> config,
        std::shared_ptr<otel::core::SpanNamer> spanNamer) override;

    IRequestProcessingEngine* getRequestProcessingEngine() override;

private:
    AgentKernel(const AgentKernel&);
    AgentKernel& operator=(const AgentKernel&);

    AgentLogger mLogger;

protected:
    std::unique_ptr<IRequestProcessingEngine> mRequestProcessingEngine;

    //Todo: Add availble resources that need to be shared among all the worker-threads
};

class WebServerContext : public IContext
{
public:
    WebServerContext(std::shared_ptr<otel::core::TenantConfig> tenantConfig);
    ~WebServerContext() = default;

    void initContext(std::shared_ptr<otel::core::SpanNamer> spanNamer) override;

    IKernel* getKernel() const override { return mAgentKernel.get();}
    std::shared_ptr<otel::core::TenantConfig> getConfig() override { return mTenantConfig;}

private:
    std::shared_ptr<otel::core::TenantConfig> mTenantConfig;
    //TODO: EUM config
protected:
    std::unique_ptr<IKernel> mAgentKernel;
};

class AgentCore : public ICore
{
public:
    AgentCore() = default;
    ~AgentCore() = default;

    bool start(
        std::shared_ptr<otel::core::TenantConfig> initConfig,
        std::shared_ptr<otel::core::SpanNamer> spanNamer,
        userAddedTenantMap& userAddedTenants) override;

    void stop() override;

    // TODO:: Make RequestProcessingEngine as interface.
    IRequestProcessingEngine* getRequestProcessor(
        std::string& name) override;

    void addContext(
        const std::string& contextName,
        std::shared_ptr<otel::core::TenantConfig> newConfig) override;

    std::shared_ptr<IContext>
        getWebServerContext(std::string& name) override;

// Member variables
private:
    AgentLogger mLogger;
    std::shared_ptr<otel::core::SpanNamer> mSpanNamer;

protected:
    std::unordered_map<std::string,
    std::shared_ptr<IContext>> mWebServerContexts;

    // Added for testability / mocking
    virtual void createContext(
        const std::string& contextName,
        std::shared_ptr<TenantConfig> config);
};

} // core
} // otel

#endif /* __AGENTCORE_H */

