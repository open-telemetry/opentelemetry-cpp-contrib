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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "AgentCore.h"
#include "mocks/mock_core.h"
#include "mocks/mock_RequestProcessingEngine.h"
#include <memory>

class FakeContext : public otel::core::WebServerContext
{
public:
	FakeContext(std::shared_ptr<otel::core::TenantConfig> config)
		: otel::core::WebServerContext(config)
	{}
	void initContext(std::shared_ptr<otel::core::SpanNamer> spanNamer)
	{
		mAgentKernel.reset(new MockAgentKernel);
	}
};

class FakeAgentCore : public otel::core::AgentCore
{
public:
	FakeAgentCore(
		std::shared_ptr<MockContext> initContext,
		std::shared_ptr<MockContext> dummyContext)
		: mInitContext(initContext)
		, mDummyContext(dummyContext)
	{}

protected:
	void createContext(
        const std::string& contextName,
        std::shared_ptr<otel::core::TenantConfig> config)
	{
		if (contextName == COREINIT_CONTEXT)
		{
			mWebServerContexts[contextName] = mInitContext;
		}
		else if (contextName == "dummyConfig")
		{
			mWebServerContexts[contextName] = mDummyContext;
		}
	}

private:
	std::shared_ptr<MockContext> mInitContext;
	std::shared_ptr<MockContext> mDummyContext;
};

class FakeKernel : public otel::core::AgentKernel
{
public:
	FakeKernel(MockRequestProcessingEngine* engine)
	: mEngine(engine)
	{}

	void initKernel(std::shared_ptr<otel::core::TenantConfig> config,
        std::shared_ptr<otel::core::SpanNamer> spanNamer)
	{
		mRequestProcessingEngine.reset(mEngine);
	}
private:
	MockRequestProcessingEngine* mEngine;
};


void fillTenantConfig(std::shared_ptr<otel::core::TenantConfig> config)
{
	config->setServiceNamespace("serviceNamespace");
    config->setServiceName("serviceName");
    config->setServiceInstanceId("serviceInstanceId");
    config->setOtelExporterType("otelExporterType");
    config->setOtelExporterEndpoint("otelExporterEndpoint");
    config->setOtelProcessorType("otelProcessorType");
    config->setOtelSamplerType("otelProcessorType");
    config->setOtelMaxQueueSize(500);
    config->setOtelScheduledDelayMillis(5000);
    config->setOtelMaxExportBatchSize(100);
}

using ::testing::Return;
using ::testing::InvokeWithoutArgs;
using testing::_;

TEST(WebserverContext, webserver_context_creation_success)
{
	auto tenantConfig = std::make_shared<otel::core::TenantConfig>();
	fillTenantConfig(tenantConfig);

    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	FakeContext context(tenantConfig);
	context.initContext(spanNamer);

	otel::core::IKernel* kernel = context.getKernel();
	EXPECT_NE(nullptr, kernel);

	MockAgentKernel* mockKernel = dynamic_cast<MockAgentKernel*>(kernel);
	EXPECT_NE(nullptr, mockKernel);

	auto config = context.getConfig();
	EXPECT_NE(nullptr, config.get());
	EXPECT_EQ(config->getServiceNamespace(), tenantConfig->getServiceNamespace());
}

TEST(AgentCore, agent_core_start_returns_true)
{
	auto initConfig = std::make_shared<otel::core::TenantConfig>();
	fillTenantConfig(initConfig);

	auto dummyConfig = std::make_shared<otel::core::TenantConfig>();
	fillTenantConfig(dummyConfig);
	dummyConfig->setServiceInstanceId("dummyInstanceId");

	std::unordered_map<
		std::string,
		std::shared_ptr<otel::core::TenantConfig>> mapTenantConfig;
	mapTenantConfig["dummyConfig"] = dummyConfig;

	auto initContext = std::make_shared<MockContext>();
	auto dummyContext = std::make_shared<MockContext>();

	EXPECT_CALL(*initContext, getConfig()).Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->std::shared_ptr<otel::core::TenantConfig>{
			return initConfig;
		}));

	EXPECT_CALL(*initContext, initContext(_)).Times(1);
	EXPECT_CALL(*dummyContext, initContext(_)).Times(1);

    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	FakeAgentCore agentCore(initContext, dummyContext);
	bool ret = agentCore.start(initConfig, spanNamer, mapTenantConfig);
	EXPECT_EQ(ret, true);
}

TEST(AgentCore, get_request_processor_returns_valid_object)
{
	auto initConfig = std::make_shared<otel::core::TenantConfig>();
	fillTenantConfig(initConfig);

	auto dummyConfig = std::make_shared<otel::core::TenantConfig>();
	fillTenantConfig(dummyConfig);
	dummyConfig->setServiceInstanceId("dummyInstanceId");

	std::unordered_map<
		std::string,
		std::shared_ptr<otel::core::TenantConfig>> mapTenantConfig;
	mapTenantConfig["dummyConfig"] = dummyConfig;

	auto initContext = std::make_shared<MockContext>();
	auto dummyContext = std::make_shared<MockContext>();

	EXPECT_CALL(*initContext, getConfig()).Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->std::shared_ptr<otel::core::TenantConfig>{
			return initConfig;
		}));

	EXPECT_CALL(*initContext, initContext(_)).Times(1);
	EXPECT_CALL(*dummyContext, initContext(_)).Times(1);

    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	FakeAgentCore agentCore(initContext, dummyContext);
	bool ret = agentCore.start(initConfig, spanNamer, mapTenantConfig);
	EXPECT_EQ(ret, true);

	MockAgentKernel mockKernel;
	MockRequestProcessingEngine mockEngine;
	EXPECT_CALL(*dummyContext, getKernel()).Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->otel::core::IKernel*{
			return &mockKernel;
		}));

	EXPECT_CALL(mockKernel, getRequestProcessingEngine()).Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->otel::core::IRequestProcessingEngine*{
			return &mockEngine;
		}));

	std::string contextName = "dummyConfig";
	auto *engine = agentCore.getRequestProcessor(contextName);
	EXPECT_NE(engine, nullptr);
}

TEST(AgentCore, get_request_processor_returns_nullptr)
{
	auto initConfig = std::make_shared<otel::core::TenantConfig>();
	fillTenantConfig(initConfig);

	auto dummyConfig = std::make_shared<otel::core::TenantConfig>();
	fillTenantConfig(dummyConfig);
	dummyConfig->setServiceInstanceId("dummyInstanceId");

	std::unordered_map<
		std::string,
		std::shared_ptr<otel::core::TenantConfig>> mapTenantConfig;
	mapTenantConfig["dummyConfig"] = dummyConfig;

	auto initContext = std::make_shared<MockContext>();
	auto dummyContext = std::make_shared<MockContext>();

	EXPECT_CALL(*initContext, getConfig()).Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->std::shared_ptr<otel::core::TenantConfig>{
			return initConfig;
		}));

	EXPECT_CALL(*initContext, initContext(_)).Times(1);
	EXPECT_CALL(*dummyContext, initContext(_)).Times(1);

    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	FakeAgentCore agentCore(initContext, dummyContext);
	bool ret = agentCore.start(initConfig, spanNamer, mapTenantConfig);
	EXPECT_EQ(ret, true);

	std::string contextName = "failConfig";
	auto *engine = agentCore.getRequestProcessor(contextName);
	EXPECT_EQ(engine, nullptr);
}

TEST(AgentCore, get_webserver_context_returns_valid_object)
{
	auto initConfig = std::make_shared<otel::core::TenantConfig>();
	fillTenantConfig(initConfig);

	auto dummyConfig = std::make_shared<otel::core::TenantConfig>();
	fillTenantConfig(dummyConfig);
	dummyConfig->setServiceInstanceId("dummyInstanceId");

	std::unordered_map<
		std::string,
		std::shared_ptr<otel::core::TenantConfig>> mapTenantConfig;
	mapTenantConfig["dummyConfig"] = dummyConfig;

	auto initContext = std::make_shared<MockContext>();
	auto dummyContext = std::make_shared<MockContext>();

	EXPECT_CALL(*initContext, getConfig()).Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->std::shared_ptr<otel::core::TenantConfig>{
			return initConfig;
		}));

	EXPECT_CALL(*initContext, initContext(_)).Times(1);
	EXPECT_CALL(*dummyContext, initContext(_)).Times(1);

    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	FakeAgentCore agentCore(initContext, dummyContext);
	bool ret = agentCore.start(initConfig, spanNamer, mapTenantConfig);
	EXPECT_EQ(ret, true);

	std::string contextName = "dummyConfig";
	auto context = agentCore.getWebServerContext(contextName);
	EXPECT_NE(context.get(), nullptr);
	EXPECT_EQ(context, dummyContext);

	contextName = "";
	context = agentCore.getWebServerContext(contextName);
	EXPECT_NE(context.get(), nullptr);
	EXPECT_EQ(context, initContext);
}

TEST(AgentCore, get_webserver_context_returns_nullptr)
{
	auto initConfig = std::make_shared<otel::core::TenantConfig>();
	fillTenantConfig(initConfig);

	auto dummyConfig = std::make_shared<otel::core::TenantConfig>();
	fillTenantConfig(dummyConfig);
	dummyConfig->setServiceInstanceId("dummyInstanceId");

	std::unordered_map<
		std::string,
		std::shared_ptr<otel::core::TenantConfig>> mapTenantConfig;
	mapTenantConfig["dummyConfig"] = dummyConfig;

	auto initContext = std::make_shared<MockContext>();
	auto dummyContext = std::make_shared<MockContext>();

	EXPECT_CALL(*initContext, getConfig()).Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->std::shared_ptr<otel::core::TenantConfig>{
			return initConfig;
		}));

	EXPECT_CALL(*initContext, initContext(_)).Times(1);
	EXPECT_CALL(*dummyContext, initContext(_)).Times(1);

    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	FakeAgentCore agentCore(initContext, dummyContext);
	bool ret = agentCore.start(initConfig, spanNamer, mapTenantConfig);
	EXPECT_EQ(ret, true);

	std::string contextName = "failConfig";
	auto context = agentCore.getWebServerContext(contextName);
	EXPECT_EQ(context.get(), nullptr);
}

TEST(AgentKernel, agent_kernel_creation_success)
{
	auto initConfig = std::make_shared<otel::core::TenantConfig>();
	fillTenantConfig(initConfig);

	MockRequestProcessingEngine *engine = new MockRequestProcessingEngine();
	testing::Mock::AllowLeak(engine);

    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	FakeKernel agentKernel(engine);
	agentKernel.initKernel(initConfig, spanNamer);
	auto *rEngine = dynamic_cast<MockRequestProcessingEngine*>
		(agentKernel.getRequestProcessingEngine());
	EXPECT_NE(rEngine, nullptr);
	EXPECT_EQ(engine, rEngine);
}

