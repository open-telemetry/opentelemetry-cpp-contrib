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

#include "AgentCore.h"

namespace otel {
namespace core {

AgentKernel::AgentKernel() :
    mLogger(getLogger(std::string(LogContext::AGENT) + ".AgentKernel"))
{
}

void
AgentKernel::initKernel(std::shared_ptr<TenantConfig> config,
    std::shared_ptr<SpanNamer> spanNamer)
{
    mRequestProcessingEngine.reset(new RequestProcessingEngine());
    mRequestProcessingEngine->init(config, spanNamer);
}

IRequestProcessingEngine*
AgentKernel::getRequestProcessingEngine()
{
    return mRequestProcessingEngine.get();
}

WebServerContext::WebServerContext
    (std::shared_ptr<TenantConfig> tenantConfig) :
    mTenantConfig(tenantConfig)
{
}

void
WebServerContext::initContext(std::shared_ptr<SpanNamer> spanNamer)
{
    mAgentKernel.reset(new AgentKernel());
    mAgentKernel->initKernel(mTenantConfig, spanNamer);
}

bool
AgentCore::start(
    std::shared_ptr<TenantConfig> initConfig,
    std::shared_ptr<SpanNamer> spanNamer,
    userAddedTenantMap& userAddedTenant)
{
    mLogger = getLogger(std::string(LogContext::AGENT) + ".AgentCore");
    if (!mLogger)
    {
        logStartupError("Agent logging was not initialized");
        return false;
    }
    LOG4CXX_DEBUG(mLogger, "Starting AgentCore with initial Config "<< *initConfig);

    mSpanNamer = spanNamer;
    createContext(COREINIT_CONTEXT, initConfig);
    mWebServerContexts[COREINIT_CONTEXT]->initContext(mSpanNamer);

    LOG4CXX_DEBUG(mLogger,
        "AgentCore::start - Initial context saved in AgentCore::WebServerContext map in key COREINIT_CONTEXT");

    //TODO: Stuff for fetching and adding all available contexts
    for (auto it = userAddedTenant.begin(); it != userAddedTenant.end(); ++it)
    {
        addContext(it->first, it->second);
    }

    LOG4CXX_INFO(mLogger, "AgentCore started");
    return true;
}

void
TenantConfig::copyAllExceptNamespaceNameId(const TenantConfig& copyfrom)
{
    std::string oldServiceNamespace = serviceNamespace;
    std::string oldServiceName = serviceName;
    std::string oldServiceInstanceId = serviceInstanceId;

    *this = copyfrom;
    serviceNamespace = oldServiceNamespace;
    serviceName = oldServiceName;
    serviceInstanceId = oldServiceInstanceId;
}

void
AgentCore::stop()
{
    LOG4CXX_INFO(mLogger, "AgentCore Stopped.");
    mLogger = nullptr;
}

std::shared_ptr<IContext>
AgentCore::getWebServerContext(
    std::string& name)
{
    // NULL or empty string are not valid context names. Use default context.
    if (0 == name.length())
    {
        name = COREINIT_CONTEXT;
    }

    auto it = mWebServerContexts.find(name);
    if (it == mWebServerContexts.end())
    {
        LOG4CXX_WARN(mLogger, "No Context found for " << name);
        return nullptr;
    }

    return it->second;
}

// This will add a new context or replace the existing one
void
AgentCore::addContext(
    const std::string& contextName,
    std::shared_ptr<TenantConfig> config)
{
    std::string initialContext {COREINIT_CONTEXT};
    std::shared_ptr<IContext> initialConfig = getWebServerContext(initialContext);
    if(initialConfig)
    {
        config->copyAllExceptNamespaceNameId(*((initialConfig.get())->getConfig()));
        createContext(contextName, config);
        mWebServerContexts[contextName]->initContext(mSpanNamer);
        LOG4CXX_DEBUG(mLogger, "Context added for " << contextName);
    }
}

void
AgentCore::createContext(
    const std::string& contextName,
    std::shared_ptr<TenantConfig> config)
{
    mWebServerContexts[contextName] = std::make_shared<WebServerContext>(config);
    std::string logMsg = "Added context: "+contextName+" Service Namespace: "
        +config->getServiceNamespace()+" Service Name: "+config->getServiceName()+
        " Instance ID: "+(config->getServiceInstanceId());
    LOG4CXX_DEBUG(mLogger, logMsg);
}

IRequestProcessingEngine*
AgentCore::getRequestProcessor(
    std::string& name)
{
    auto wsContext = getWebServerContext(name);
    if (!wsContext)
    {
        LOG4CXX_WARN(mLogger, "No context found for [" << name << "]");
        return nullptr;
    }

    return wsContext->getKernel()->getRequestProcessingEngine();
}

} // core
} // otel

