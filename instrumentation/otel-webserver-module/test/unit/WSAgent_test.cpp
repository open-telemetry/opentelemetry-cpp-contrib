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
#include "api/WSAgent.h"
#include "AgentCore.h"
#include "api/ApiUtils.h"
#include "mocks/mock_core.h"
#include "mocks/mock_RequestProcessingEngine.h"
#include "api/Payload.h"
#include <unordered_map>

class FakeWSAgent : public otel::core::WSAgent
{
public:
	MockAgentCore* getMockAgentCore() { return dynamic_cast<MockAgentCore*>(mAgentCore.get());}
	MockApiUtils* getMockApiUtils() {return dynamic_cast<MockApiUtils*>(mApiUtils.get());}

	void initDependency() override
	{
		if (mAgentCore.get() == nullptr) {
			mAgentCore.reset(new MockAgentCore());
		}
		if (mApiUtils.get() == nullptr) {
        	mApiUtils.reset(new MockApiUtils());
    	}
	}

	void addUserTenant(
        const std::string& context, std::shared_ptr<otel::core::TenantConfig> tenantConfig)
	{
		mUserAddedTenant[context] = tenantConfig;
	}
};

using ::testing::Return;

TEST(WSAgent, initialise_WSAgent_returns_success)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	bool isInit = agent.isInitialised();
	EXPECT_EQ(isInit, false);

	OTEL_SDK_ENV_RECORD* env = (OTEL_SDK_ENV_RECORD*)calloc(2, sizeof(OTEL_SDK_ENV_RECORD));
	env[0].name = "Key1";
	env[0].value = "Value1";

	env[1].name = "Key2";
	env[1].value = "Value2";

	OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));

	EXPECT_CALL(*apiUtils, ReadFromPassedSettings(_,_,_,_))
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));
	EXPECT_CALL(*agentCore, start(_,_,_))
		.Times(1)
		.WillOnce(Return(true));

	res = agent.init(env, 2);
	EXPECT_EQ(res, OTEL_SUCCESS);

	isInit = agent.isInitialised();
	EXPECT_EQ(isInit, true);

	free(env);
}

TEST(WSAgent, initialise_WSAgent_returns_success_if_already_initialised)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	OTEL_SDK_ENV_RECORD* env = (OTEL_SDK_ENV_RECORD*)calloc(2, sizeof(OTEL_SDK_ENV_RECORD));
	env[0].name = "Key1";
	env[0].value = "Value1";

	env[1].name = "Key2";
	env[1].value = "Value2";

	OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));

	EXPECT_CALL(*apiUtils, ReadFromPassedSettings(_,_,_,_))
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));
	EXPECT_CALL(*agentCore, start(_,_,_))
		.Times(1)
		.WillOnce(Return(true));

	res = agent.init(env, 2);
	EXPECT_EQ(res, OTEL_SUCCESS);

	bool isInit = agent.isInitialised();
	EXPECT_EQ(isInit, true);

	res = agent.init(env, 2);
	EXPECT_EQ(res, OTEL_STATUS(already_initialized));

	isInit = agent.isInitialised();
	EXPECT_EQ(isInit, true);

	free(env);
}

TEST(WSAgent, initialise_WSAgent_returns_fail_on_init_boilerplate_failure)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	OTEL_SDK_ENV_RECORD* env = (OTEL_SDK_ENV_RECORD*)calloc(2, sizeof(OTEL_SDK_ENV_RECORD));
	env[0].name = "Key1";
	env[0].value = "Value1";

	env[1].name = "Key2";
	env[1].value = "Value2";

	OTEL_SDK_STATUS_CODE res = OTEL_STATUS(fail);

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));

	res = agent.init(env, 2);
	EXPECT_EQ(res, OTEL_STATUS(fail));

	free(env);
}

TEST(WSAgent, initialise_WSAgent_returns_fail_on_agentcore_start_failure)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	OTEL_SDK_ENV_RECORD* env = (OTEL_SDK_ENV_RECORD*)calloc(2, sizeof(OTEL_SDK_ENV_RECORD));
	env[0].name = "Key1";
	env[0].value = "Value1";

	env[1].name = "Key2";
	env[1].value = "Value2";

	OTEL_SDK_STATUS_CODE res1 = OTEL_SUCCESS;
	OTEL_SDK_STATUS_CODE res2 = OTEL_STATUS(fail);

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res1;
		}));
	EXPECT_CALL(*apiUtils, ReadFromPassedSettings(_,_,_,_))
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res2;
		}));

	OTEL_SDK_STATUS_CODE res = agent.init(env, 2);
	EXPECT_EQ(res, OTEL_STATUS(fail));

	free(env);
}

TEST(WSAgent, initialise_WSAgent_returns_agent_failed_to_start_on_read_config_failure)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	OTEL_SDK_ENV_RECORD* env = (OTEL_SDK_ENV_RECORD*)calloc(2, sizeof(OTEL_SDK_ENV_RECORD));
	env[0].name = "Key1";
	env[0].value = "Value1";

	env[1].name = "Key2";
	env[1].value = "Value2";

	OTEL_SDK_STATUS_CODE res1 = OTEL_SUCCESS;

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res1;
		}));
	EXPECT_CALL(*apiUtils, ReadFromPassedSettings(_,_,_,_))
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res1;
		}));
	EXPECT_CALL(*agentCore, start(_,_,_))
		.Times(1)
		.WillOnce(Return(false));

	OTEL_SDK_STATUS_CODE res = agent.init(env, 2);
	EXPECT_EQ(res, OTEL_STATUS(agent_failed_to_start));

	free(env);
}

/*TEST(WSAgent, initialise_WSAgent_returns_success_on_ReadFromEnvinronment_returns_success)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	OTEL_SDK_ENV_RECORD* env = NULL;

	OTEL_SDK_STATUS_CODE res1 = OTEL_SUCCESS;

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res1;
		}));
	EXPECT_CALL(*apiUtils, ReadFromEnvinronment(_))
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res1;
		}));
	EXPECT_CALL(*agentCore, start(_,_,_))
		.Times(1)
		.WillOnce(Return(true));

	OTEL_SDK_STATUS_CODE res = agent.init(env, 0);
	EXPECT_EQ(res, OTEL_STATUS(success));
}*/

TEST(WSAgent, start_request_executes_successfully)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	OTEL_SDK_ENV_RECORD* env = (OTEL_SDK_ENV_RECORD*)calloc(1, sizeof(OTEL_SDK_ENV_RECORD));
	env[0].name = "Key1";
	env[0].value = "Value1";

	OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));

	EXPECT_CALL(*apiUtils, ReadFromPassedSettings(_,_,_,_))
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));
	EXPECT_CALL(*agentCore, start(_,_,_))
		.Times(1)
		.WillOnce(Return(true));

	res = agent.init(env, 1);
	EXPECT_EQ(res, OTEL_SUCCESS);

	bool isInit = agent.isInitialised();
	EXPECT_EQ(isInit, true);

	MockRequestProcessingEngine mockProcessor;
	auto payload = std::unique_ptr<otel::core::RequestPayload>(new otel::core::RequestPayload());

	std::string contextName {"contextName"};
	EXPECT_CALL(*agentCore, getRequestProcessor(contextName))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->otel::core::IRequestProcessingEngine*{
			return &mockProcessor;
		}));

	EXPECT_CALL(mockProcessor, startRequest(_,_,_))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));

	void* reqHandle = nullptr;
	const char* wscontext = "contextName";
	res = agent.startRequest(wscontext, payload.get(), &reqHandle);
	EXPECT_EQ(res, OTEL_SUCCESS);

	free(env);
}

TEST(WSAgent, start_request_returns_fail)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	OTEL_SDK_ENV_RECORD* env = (OTEL_SDK_ENV_RECORD*)calloc(1, sizeof(OTEL_SDK_ENV_RECORD));
	env[0].name = "Key1";
	env[0].value = "Value1";

	OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));

	EXPECT_CALL(*apiUtils, ReadFromPassedSettings(_,_,_,_))
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));
	EXPECT_CALL(*agentCore, start(_,_,_))
		.Times(1)
		.WillOnce(Return(true));

	res = agent.init(env, 1);
	EXPECT_EQ(res, OTEL_SUCCESS);

	bool isInit = agent.isInitialised();
	EXPECT_EQ(isInit, true);

	MockRequestProcessingEngine mockProcessor;
	auto payload = std::unique_ptr<otel::core::RequestPayload>(new otel::core::RequestPayload());

	std::string contextName {"contextName"};
	EXPECT_CALL(*agentCore, getRequestProcessor(contextName))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->otel::core::IRequestProcessingEngine*{
			return nullptr;
		}));

	void* reqHandle = nullptr;
	const char* wscontext = "contextName";
	res = agent.startRequest(wscontext, payload.get(), &reqHandle);
	EXPECT_EQ(res, OTEL_STATUS(fail));



	std::string contextName1 {"contextName1"};
	EXPECT_CALL(*agentCore, getRequestProcessor(contextName1))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->otel::core::IRequestProcessingEngine*{
			return &mockProcessor;
		}));

	OTEL_SDK_STATUS_CODE res1 = OTEL_STATUS(fail);
	EXPECT_CALL(mockProcessor, startRequest(_,_,_))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res1;
		}));

	const char* wscontext1 = "contextName1";
	res = agent.startRequest(wscontext1, payload.get(), &reqHandle);
	EXPECT_EQ(res, OTEL_STATUS(fail));

	free(env);
}


TEST(WSAgent, end_request_returns_success)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	OTEL_SDK_ENV_RECORD* env = (OTEL_SDK_ENV_RECORD*)calloc(1, sizeof(OTEL_SDK_ENV_RECORD));
	env[0].name = "Key1";
	env[0].value = "Value1";

	OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));

	EXPECT_CALL(*apiUtils, ReadFromPassedSettings(_,_,_,_))
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));
	EXPECT_CALL(*agentCore, start(_,_,_))
		.Times(1)
		.WillOnce(Return(true));

	res = agent.init(env, 1);
	EXPECT_EQ(res, OTEL_SUCCESS);

	bool isInit = agent.isInitialised();
	EXPECT_EQ(isInit, true);

	MockRequestProcessingEngine mockProcessor;

	void* reqHandle = nullptr;
	std::string contextName1 {"contextName1"};
	EXPECT_CALL(*agentCore, getRequestProcessor(contextName1))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->otel::core::IRequestProcessingEngine*{
			return &mockProcessor;
		}));

	OTEL_SDK_STATUS_CODE res1 = OTEL_SUCCESS;
	EXPECT_CALL(mockProcessor, endRequest(_,_,_))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res1;
		}));

	auto reqCtx = new otel::core::RequestContext();
	reqCtx->setContextName(contextName1);
	reqHandle = static_cast<void*>(reqCtx);
	const char* err = nullptr;
	res = agent.endRequest(reqHandle, err);
	EXPECT_EQ(res, OTEL_SUCCESS);

	free(env);
	delete reqCtx;
}

TEST(WSAgent, start_interaction_returns_success)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	OTEL_SDK_ENV_RECORD* env = (OTEL_SDK_ENV_RECORD*)calloc(1, sizeof(OTEL_SDK_ENV_RECORD));
	env[0].name = "Key1";
	env[0].value = "Value1";

	OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));

	EXPECT_CALL(*apiUtils, ReadFromPassedSettings(_,_,_,_))
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));
	EXPECT_CALL(*agentCore, start(_,_,_))
		.Times(1)
		.WillOnce(Return(true));

	res = agent.init(env, 1);
	EXPECT_EQ(res, OTEL_SUCCESS);

	bool isInit = agent.isInitialised();
	EXPECT_EQ(isInit, true);

	MockRequestProcessingEngine mockProcessor;

	void* reqHandle = nullptr;
	std::string contextName1 {"contextName1"};
	EXPECT_CALL(*agentCore, getRequestProcessor(contextName1))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->otel::core::IRequestProcessingEngine*{
			return &mockProcessor;
		}));

	OTEL_SDK_STATUS_CODE res1 = OTEL_SUCCESS;
	EXPECT_CALL(mockProcessor, startInteractionImpl(_,_))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res1;
		}));

	otel::core::InteractionPayload* payload = nullptr;
    std::unordered_map<std::string, std::string> propagationHeaders;

	auto reqCtx = new otel::core::RequestContext();
	reqCtx->setContextName(contextName1);
	reqHandle = static_cast<void*>(reqCtx);

	res = agent.startInteraction(reqHandle, payload, propagationHeaders);
	EXPECT_EQ(res, OTEL_SUCCESS);

	free(env);
	delete reqCtx;
}

TEST(WSAgent, end_interaction_returns_success)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	OTEL_SDK_ENV_RECORD* env = (OTEL_SDK_ENV_RECORD*)calloc(1, sizeof(OTEL_SDK_ENV_RECORD));
	env[0].name = "Key1";
	env[0].value = "Value1";

	OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));

	EXPECT_CALL(*apiUtils, ReadFromPassedSettings(_,_,_,_))
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));
	EXPECT_CALL(*agentCore, start(_,_,_))
		.Times(1)
		.WillOnce(Return(true));

	res = agent.init(env, 1);
	EXPECT_EQ(res, OTEL_SUCCESS);

	bool isInit = agent.isInitialised();
	EXPECT_EQ(isInit, true);

	MockRequestProcessingEngine mockProcessor;

	void* reqHandle = nullptr;
	std::string contextName1 {"contextName1"};
	EXPECT_CALL(*agentCore, getRequestProcessor(contextName1))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->otel::core::IRequestProcessingEngine*{
			return &mockProcessor;
		}));

	OTEL_SDK_STATUS_CODE res1 = OTEL_SUCCESS;
	EXPECT_CALL(mockProcessor, endInteraction(_,_,_))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res1;
		}));

	otel::core::EndInteractionPayload* payload = nullptr;

	auto reqCtx = new otel::core::RequestContext();
	reqCtx->setContextName(contextName1);
	reqHandle = static_cast<void*>(reqCtx);

	res = agent.endInteraction(reqHandle, true, payload);
	EXPECT_EQ(res, OTEL_SUCCESS);

	free(env);
	delete reqCtx;
}

TEST(WSAgent, end_interaction_returns_failure)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	OTEL_SDK_ENV_RECORD* env = (OTEL_SDK_ENV_RECORD*)calloc(1, sizeof(OTEL_SDK_ENV_RECORD));
	env[0].name = "Key1";
	env[0].value = "Value1";

	OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));

	EXPECT_CALL(*apiUtils, ReadFromPassedSettings(_,_,_,_))
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));
	EXPECT_CALL(*agentCore, start(_,_,_))
		.Times(1)
		.WillOnce(Return(true));

	res = agent.init(env, 1);
	EXPECT_EQ(res, OTEL_SUCCESS);

	bool isInit = agent.isInitialised();
	EXPECT_EQ(isInit, true);

	MockRequestProcessingEngine mockProcessor;

	void* reqHandle = nullptr;
	std::string contextName1 {"contextName1"};
	EXPECT_CALL(*agentCore, getRequestProcessor(contextName1))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->otel::core::IRequestProcessingEngine*{
			return nullptr;
		}));

	otel::core::EndInteractionPayload* payload = nullptr;
	auto reqCtx = new otel::core::RequestContext();
	reqCtx->setContextName(contextName1);
	reqHandle = static_cast<void*>(reqCtx);

	res = agent.endInteraction(reqHandle, true, payload);
	EXPECT_EQ(res, OTEL_STATUS(fail));
	delete reqCtx;

	std::string contextName2 {"contextName2"};
	EXPECT_CALL(*agentCore, getRequestProcessor(contextName2))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->otel::core::IRequestProcessingEngine*{
			return &mockProcessor;
		}));

	OTEL_SDK_STATUS_CODE res2 = OTEL_STATUS(fail);
	EXPECT_CALL(mockProcessor, endInteraction(_,_,_))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res2;
		}));

	reqCtx = new otel::core::RequestContext();
	reqCtx->setContextName(contextName2);
	reqHandle = static_cast<void*>(reqCtx);
	res = agent.endInteraction(reqHandle, true, payload);
	EXPECT_EQ(res, OTEL_STATUS(fail));

	free(env);

}

TEST(WSAgent, add_wscontext_to_core_returns_failure_with_core_uninitialised)
{
	FakeWSAgent agent;
	agent.initDependency();

	bool isInit = agent.isInitialised();
	EXPECT_EQ(isInit, false);

	using testing::_;
	using ::testing::InvokeWithoutArgs;

	std::string contextName = "";
	otel::core::WSContextConfig contextConfig;

	int res = agent.addWSContextToCore(contextName.c_str(), &contextConfig); // contextName is NULL
	EXPECT_EQ(res, -1);

	contextName = COREINIT_CONTEXT;

	res = agent.addWSContextToCore(contextName.c_str(), &contextConfig); // contextName is COREINIT_CONTEXT
	EXPECT_EQ(res, -1);

	contextName = "testContextName";

	res = agent.addWSContextToCore(contextName.c_str(), nullptr); // contextConfig is NULL
	EXPECT_EQ(res, -1);

	contextConfig.serviceNamespace = "";
	contextConfig.serviceName = "testName";
	contextConfig.serviceInstanceId = "testId";

	res = agent.addWSContextToCore(contextName.c_str(), &contextConfig); // serviceNamespace is NULL
	EXPECT_EQ(res, -1);

	contextConfig.serviceNamespace = "testNamespace";
	contextConfig.serviceName = "";

	res = agent.addWSContextToCore(contextName.c_str(), &contextConfig); // serviceName is NULL
	EXPECT_EQ(res, -1);

	contextConfig.serviceName = "testName";
	contextConfig.serviceInstanceId = "";

	res = agent.addWSContextToCore(contextName.c_str(), &contextConfig); // serviceInstanceId is NULL
	EXPECT_EQ(res, -1);

	contextConfig.serviceInstanceId = "testId";

	std::shared_ptr<otel::core::TenantConfig> tenant = std::make_shared<otel::core::TenantConfig>();
    tenant->setServiceNamespace(contextConfig.serviceNamespace);
    tenant->setServiceName(contextConfig.serviceName);
    tenant->setServiceInstanceId(contextConfig.serviceInstanceId);

    agent.addUserTenant(contextName, tenant);

    res = agent.addWSContextToCore(contextName.c_str(), &contextConfig); // context already exists
	EXPECT_EQ(res, 0);
}

TEST(WSAgent, add_wscontext_to_core_returns_failure_with_core_initialised)
{
	FakeWSAgent agent;
	agent.initDependency();

	MockAgentCore* agentCore = agent.getMockAgentCore();
	MockApiUtils* apiUtils = agent.getMockApiUtils();
	testing::Mock::AllowLeak(agentCore);
	testing::Mock::AllowLeak(apiUtils);

	OTEL_SDK_ENV_RECORD* env = (OTEL_SDK_ENV_RECORD*)calloc(1, sizeof(OTEL_SDK_ENV_RECORD));
	env[0].name = "Key1";
	env[0].value = "Value1";

	OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;

	using testing::_;
	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*apiUtils, init_boilerplate())
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));

	EXPECT_CALL(*apiUtils, ReadFromPassedSettings(_,_,_,_))
		.WillOnce(InvokeWithoutArgs([&]()->OTEL_SDK_STATUS_CODE{
			return res;
		}));
	EXPECT_CALL(*agentCore, start(_,_,_))
		.Times(1)
		.WillOnce(Return(true));

	res = agent.init(env, 1);
	EXPECT_EQ(res, OTEL_SUCCESS);

	bool isInit = agent.isInitialised();
	EXPECT_EQ(isInit, true);

	std::string contextName1 = "testContextName1";
	otel::core::WSContextConfig contextConfig;

	contextConfig.serviceNamespace = "testNamespace";
	contextConfig.serviceName = "testName";
	contextConfig.serviceInstanceId = "testId";

	std::shared_ptr<MockContext> mockContext = std::make_shared<MockContext> ();

	EXPECT_CALL(*agentCore, getWebServerContext(contextName1))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->std::shared_ptr<otel::core::IContext>{
			return mockContext;
		}));

	std::shared_ptr<otel::core::TenantConfig> tenant = std::make_shared<otel::core::TenantConfig> ();
	EXPECT_CALL(*mockContext, getConfig())
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->std::shared_ptr<otel::core::TenantConfig>{
			return tenant;
		}));

	int res2 = agent.addWSContextToCore(contextName1.c_str(), &contextConfig);
	EXPECT_EQ(res2, 0);

	std::string contextName2 = "testContextName2";
	EXPECT_CALL(*agentCore, getWebServerContext(contextName2))
		.Times(1)
		.WillOnce(InvokeWithoutArgs([&]()->std::shared_ptr<otel::core::IContext>{
			return nullptr;
		}));

	EXPECT_CALL(*agentCore, addContext(contextName2,_))
		.Times(1);

	res2 = agent.addWSContextToCore(contextName2.c_str(), &contextConfig);
	EXPECT_EQ(res2, 0);

	free(env);
}
