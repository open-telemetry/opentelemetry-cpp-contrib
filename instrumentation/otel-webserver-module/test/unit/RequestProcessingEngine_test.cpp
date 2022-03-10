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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "api/RequestProcessingEngine.h"
#include "mocks/mock_sdkwrapper.h"
#include "api/TenantConfig.h"
#include "api/Payload.h"

#include <unordered_map>

class FakeRequestProcessingEngine :  appd::core::RequestProcessingEngine {
public:
	using Base = appd::core::RequestProcessingEngine;
	using Base::startRequest;
	using Base::endRequest;
	using Base::startInteraction;
	using Base::endInteraction;
	FakeRequestProcessingEngine() = default;
	void init(std::shared_ptr<appd::core::TenantConfig>& config,
        std::shared_ptr<appd::core::SpanNamer> spanNamer) override {
		m_sdkWrapper.reset(new MockSdkWrapper);
        m_spanNamer = spanNamer;
	}

	MockSdkWrapper* getMockSdkWrapper() {
		return (MockSdkWrapper*)(m_sdkWrapper.get());
	}

};

using ::testing::Return;

MATCHER_P(HasStringVal, value, "") {
  return opentelemetry::nostd::get<opentelemetry::nostd::string_view>(arg) == value;
}


MATCHER_P(HasIntValue, value, "") {
  return opentelemetry::nostd::get<int>(arg) == value;
}

MATCHER_P(HasBoolValue, value, "") {
  return opentelemetry::nostd::get<bool>(arg) == value;
}

TEST(TestRequestProcessingEngine, StartRequest)
{
	FakeRequestProcessingEngine engine;
	std::shared_ptr<appd::core::TenantConfig> config;
    auto spanNamer = std::make_shared<appd::core::SpanNamer>();
	engine.init(config, spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);
	std::string wscontext = "ws_context";
	appd::core::RequestPayload payload;
	payload.set_http_headers("key", "value");
	payload.set_uri("dummy_span");
	payload.set_request_protocol("GET");

	appd::core::sdkwrapper::OtelKeyValueMap keyValueMap;
  	keyValueMap["request_protocol"] = "GET";

  	std::shared_ptr<appd::core::sdkwrapper::IScopedSpan> span;
  	span.reset(new MockScopedSpan);

	// sdkwrapper's create span function should be called
	EXPECT_CALL(*sdkWrapper, CreateSpan("dummy_span",
		appd::core::sdkwrapper::SpanKind::SERVER,
		keyValueMap,
		payload.get_http_headers())).
	WillOnce(Return(span));

	int* dummy = new int(2);
	void* reqHandle = dummy;
	auto res = engine.startRequest("ws_context", &payload, &reqHandle);
	EXPECT_EQ(res, APPD_SUCCESS);

	auto* reqContext = (appd::core::RequestContext*)(reqHandle);
	ASSERT_TRUE(reqContext);
	EXPECT_TRUE(reqContext->rootSpan());
	EXPECT_EQ(reqContext->getContextName(), "ws_context");
	EXPECT_FALSE(reqContext->hasActiveInteraction());

  delete(dummy);
}


TEST(TestRequestProcessingEngine, StartRequestInvalidParams)
{
	FakeRequestProcessingEngine engine;
    auto spanNamer = std::make_shared<appd::core::SpanNamer>();
	std::shared_ptr<appd::core::TenantConfig> config;
	engine.init(config, spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);
	std::string wscontext = "ws_context";
	appd::core::RequestPayload payload;

	auto res = engine.startRequest("ws_context", &payload, nullptr);
	EXPECT_EQ(res, APPD_STATUS(handle_pointer_is_null));

	int* intPtr = new int(2);
	void* reqHandle = intPtr;
	res = engine.startRequest("ws_context", nullptr, &reqHandle);
	EXPECT_EQ(res, APPD_STATUS(payload_reflector_is_null));
	delete(intPtr);
}


TEST(TestRequestProcessingEngine, EndRequest)
{
	FakeRequestProcessingEngine engine;
    auto spanNamer = std::make_shared<appd::core::SpanNamer>();
	std::shared_ptr<appd::core::TenantConfig> config;
	engine.init(config, spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);
	std::shared_ptr<appd::core::sdkwrapper::IScopedSpan> rootSpan;
  	rootSpan.reset(new MockScopedSpan);
	auto* rContext = new appd::core::RequestContext(rootSpan);

	// adding some unfinished interactions
	std::shared_ptr<appd::core::sdkwrapper::IScopedSpan> interactionSpan1, interactionSpan2;
  	interactionSpan1.reset(new MockScopedSpan);
  	interactionSpan2.reset(new MockScopedSpan);
	rContext->addInteraction(interactionSpan1);
	rContext->addInteraction(interactionSpan2);

	auto getMockSpan = [](std::shared_ptr<appd::core::sdkwrapper::IScopedSpan> span) {
		return (MockScopedSpan*)(span.get());
	};

	// interactionSpans' End should be called in LIFO order followed by root span.
	EXPECT_CALL(*getMockSpan(interactionSpan2), End()).
	Times(1);

	EXPECT_CALL(*getMockSpan(interactionSpan1), End()).
	Times(1);

	EXPECT_CALL(*getMockSpan(rootSpan), AddAttribute("error", HasBoolValue(true))).Times(1);
	EXPECT_CALL(*getMockSpan(rootSpan), AddAttribute("error_description", HasStringVal("error_msg"))).Times(1);

	EXPECT_CALL(*getMockSpan(rootSpan), End()).
	Times(1);

	auto res = engine.endRequest(rContext, "error_msg");
	EXPECT_EQ(res, APPD_SUCCESS);
}

TEST(TestRequestProcessingEngine, EndRequestInvalidParams)
{
	FakeRequestProcessingEngine engine;
	std::shared_ptr<appd::core::TenantConfig> config;
    auto spanNamer = std::make_shared<appd::core::SpanNamer>();
	engine.init(config,spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);
	auto res = engine.endRequest(nullptr, "error msg");
	EXPECT_EQ(res, APPD_STATUS(fail));
}


TEST(TestRequestProcessingEngine, StartInteraction)
{
	FakeRequestProcessingEngine engine;
	std::shared_ptr<appd::core::TenantConfig> config;
    auto spanNamer = std::make_shared<appd::core::SpanNamer>();
	engine.init(config,spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);

	appd::core::InteractionPayload payload;
	payload.moduleName = "module";
	payload.phaseName = "phase";

	appd::core::sdkwrapper::OtelKeyValueMap keyValueMap;
	keyValueMap["interactionType"] = "EXIT_CALL";

	std::shared_ptr<appd::core::sdkwrapper::IScopedSpan> span;
  	span.reset(new MockScopedSpan);

	std::unordered_map<std::string, std::string> propagationHeaders;
	std::unordered_map<std::string, std::string> emptyHeaders;

	// sdkwrapper's create span function should be called
	EXPECT_CALL(*sdkWrapper, CreateSpan("module_phase",
		appd::core::sdkwrapper::SpanKind::CLIENT,
		keyValueMap, emptyHeaders)).
	WillOnce(Return(span));

	// call to populatePropagationHeader of sdkWrapper
	EXPECT_CALL(*sdkWrapper, PopulatePropagationHeaders(propagationHeaders)).
	Times(1);

	std::shared_ptr<appd::core::sdkwrapper::IScopedSpan> rootSpan;
  	rootSpan.reset(new MockScopedSpan);
	auto* rContext = new appd::core::RequestContext(rootSpan);

	auto res = engine.startInteraction((void*)(rContext), &payload, propagationHeaders);
	EXPECT_EQ(res, APPD_SUCCESS);

	// Interaction will be added in requestContext
	EXPECT_TRUE(rContext->hasActiveInteraction());
}

TEST(TestRequestProcessingEngine, StartInteractionInvalidParams)
{
	FakeRequestProcessingEngine engine;
	std::shared_ptr<appd::core::TenantConfig> config;
    auto spanNamer = std::make_shared<appd::core::SpanNamer>();
	engine.init(config,spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);

	appd::core::InteractionPayload payload;
	std::unordered_map<std::string, std::string> propagationHeaders;

	// invalid request context
	auto res = engine.startInteraction(nullptr, &payload, propagationHeaders);
	EXPECT_EQ(res, APPD_STATUS(handle_pointer_is_null));

	std::shared_ptr<appd::core::sdkwrapper::IScopedSpan> rootSpan;
  	rootSpan.reset(new MockScopedSpan);
	auto* rContext = new appd::core::RequestContext(rootSpan);

	// invalid payload
	res = engine.startInteraction((void*)(rContext), nullptr, propagationHeaders);
	EXPECT_EQ(res, APPD_STATUS(payload_reflector_is_null));
}

TEST(TestRequestProcessingEngine, EndInteraction)
{
	FakeRequestProcessingEngine engine;
	std::shared_ptr<appd::core::TenantConfig> config;
    auto spanNamer = std::make_shared<appd::core::SpanNamer>();
	engine.init(config,spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);

	std::unordered_map<std::string, std::string> propagationHeaders;
	appd::core::InteractionPayload startPayload;
	std::shared_ptr<appd::core::sdkwrapper::IScopedSpan> rootSpan;
  	rootSpan.reset(new MockScopedSpan);
	auto* rContext = new appd::core::RequestContext(rootSpan);
	std::shared_ptr<appd::core::sdkwrapper::IScopedSpan> interactionSpan;
  	interactionSpan.reset(new MockScopedSpan);
  	rContext->addInteraction(interactionSpan);

	appd::core::EndInteractionPayload payload;
	payload.errorCode = 403;
	payload.errorMsg = "error_msg";
	payload.backendName = "backend_one";
	payload.backendType = "backend_type";

	auto span = rContext->lastActiveInteraction();
  EXPECT_CALL(*(MockScopedSpan*)(span.get()), AddAttribute("error", HasBoolValue(true))).
     Times(1);
  EXPECT_CALL(*(MockScopedSpan*)(span.get()), AddAttribute("error_code", HasIntValue(403))).
     Times(1);


	// attributes should be added
	std::unordered_map<std::string, std::string> keyVals = {
		{"error_msg", "error_msg"}, {"backend_name", "backend_one"}, {"backend_type", "backend_type"}
	};

	for (auto& keyVal : keyVals) {
		EXPECT_CALL(*(MockScopedSpan*)(span.get()), AddAttribute(keyVal.first, HasStringValue(keyVal.second))).
		  Times(1);
	}

	// interactionSpan's End should be called.
	EXPECT_CALL(*(MockScopedSpan*)(span.get()), End()).
	Times(1);

	auto res = engine.endInteraction(rContext, false, &payload);
	EXPECT_EQ(res, APPD_SUCCESS);
}

TEST(TestRequestProcessingEngine, EndInteractionInvalidParams)
{
	FakeRequestProcessingEngine engine;
	std::shared_ptr<appd::core::TenantConfig> config;
    auto spanNamer = std::make_shared<appd::core::SpanNamer>();
	engine.init(config, spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);

	// invalid handle
	auto res = engine.endInteraction(nullptr, true, nullptr);
	EXPECT_EQ(res, APPD_STATUS(handle_pointer_is_null));

	// invalid payload case
	std::shared_ptr<appd::core::sdkwrapper::IScopedSpan> rootSpan;
  	rootSpan.reset(new MockScopedSpan);
	auto* rContext = new appd::core::RequestContext(rootSpan);

	res = engine.endInteraction((void*)(rContext), true, nullptr);
	EXPECT_EQ(res, APPD_STATUS(payload_reflector_is_null));

	// no interaction in the context
	appd::core::EndInteractionPayload payload;
	res = engine.endInteraction(rContext, true, &payload);
	EXPECT_EQ(res, APPD_STATUS(invalid_context));
}

