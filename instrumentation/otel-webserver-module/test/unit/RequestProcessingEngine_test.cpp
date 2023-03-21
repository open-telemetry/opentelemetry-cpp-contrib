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
#include "api/RequestProcessingEngine.h"
#include "mocks/mock_sdkwrapper.h"
#include "api/TenantConfig.h"
#include "api/Payload.h"
#include "sdkwrapper/SdkConstants.h"

#include <unordered_map>

using namespace otel::core::sdkwrapper;

class FakeRequestProcessingEngine :  otel::core::RequestProcessingEngine {
public:
	using Base = otel::core::RequestProcessingEngine;
	using Base::startRequest;
	using Base::endRequest;
	using Base::startInteraction;
	using Base::endInteraction;
	FakeRequestProcessingEngine() = default;
	void init(std::shared_ptr<otel::core::TenantConfig>& config,
        std::shared_ptr<otel::core::SpanNamer> spanNamer) override {
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

MATCHER_P(HasMapVal, value, "") {

	using namespace opentelemetry;
	otel::core::sdkwrapper::OtelKeyValueMap argkeyValueMap = arg;
	bool valueMatches = true;
	for(auto &argKey:argkeyValueMap){
		
		auto argkeyValue = argKey.second;
		auto keyValue = argkeyValue;
		auto itr = value.find(argKey.first);

		if(itr != value.end()){
			keyValue=itr->second;
		}
		else{
			valueMatches=false;
			break;
		}

		if(keyValue.index()!=argkeyValue.index()){
			valueMatches=false;
			break;
		}

/* Only for data types have been covered pertaining to this Test but if needed more if-else for other data
   types have to be added*/

		if (nostd::holds_alternative<int64_t>(keyValue)){	
    		if(nostd::get<int64_t>(argkeyValue) != nostd::get<int64_t>(keyValue)){
    			valueMatches = false;
    		}
    }
    else if (nostd::holds_alternative<nostd::string_view>(keyValue)){
    		
    		if(nostd::get<nostd::string_view>(argkeyValue) != nostd::get<nostd::string_view>(keyValue)){
    			  valueMatches = false;	
    		}	
    }
    else if (nostd::holds_alternative<int32_t>(keyValue)){
    		if(nostd::get<int32_t>(argkeyValue) != nostd::get<int32_t>(keyValue))
    			valueMatches = false;
    }
    else if (nostd::holds_alternative<bool>(keyValue))
    {
    		if(nostd::get<bool>(argkeyValue) != nostd::get<bool>(keyValue))
    			valueMatches = false;
    }
	}

	return valueMatches;

}

MATCHER_P(HasIntValue, value, "") {
  return opentelemetry::nostd::get<int>(arg) == value;
}

MATCHER_P(HasLongIntValue, value, "") {
	
	return opentelemetry::nostd::get<int64_t>(arg) == value;
}

MATCHER_P(HasUnsignedIntValue, value, "") {

	return opentelemetry::nostd::get<uint32_t>(arg) == value;
}

MATCHER_P(HasBoolValue, value, "") {
  return opentelemetry::nostd::get<bool>(arg) == value;
}

TEST(TestRequestProcessingEngine, StartRequest)
{
	FakeRequestProcessingEngine engine;
	std::shared_ptr<otel::core::TenantConfig> config;
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	engine.init(config, spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);
	std::string wscontext = "ws_context";
	otel::core::RequestPayload payload;
	payload.set_http_headers("key", "value");
	payload.set_request_protocol("GET");
	payload.set_uri("dummy_span");
	payload.set_server_name("localhost");
	payload.set_host("host");
  payload.set_http_request_method("GET");
  payload.set_scheme("http");
  payload.set_target("target");
  payload.set_flavor("1.1");
  payload.set_client_ip("clientip");
  payload.set_port(80);


	otel::core::sdkwrapper::OtelKeyValueMap keyValueMap;
  keyValueMap[kAttrRequestProtocol] = (opentelemetry::nostd::string_view)"GET";
  keyValueMap[kAttrHTTPServerName] = (opentelemetry::nostd::string_view)"localhost";
  keyValueMap[kAttrHTTPMethod] = (opentelemetry::nostd::string_view)"GET";
  keyValueMap[kAttrNetHostName] =(opentelemetry::nostd::string_view)"host";
  keyValueMap[kAttrNETHostPort] = (long)80;
  keyValueMap[kAttrHTTPScheme] = (opentelemetry::nostd::string_view)"http";
  keyValueMap[kAttrHTTPTarget] = (opentelemetry::nostd::string_view)"target";
  keyValueMap[kAttrHTTPFlavor] = (opentelemetry::nostd::string_view)"1.1";
  keyValueMap[kAttrHTTPClientIP] = (opentelemetry::nostd::string_view)"clientip";
	std::shared_ptr<otel::core::sdkwrapper::IScopedSpan> span;
	span.reset(new MockScopedSpan);

	// sdkwrapper's create span function should be called
	EXPECT_CALL(*sdkWrapper, CreateSpan("dummy_span",
		otel::core::sdkwrapper::SpanKind::SERVER,
		HasMapVal(keyValueMap),
		payload.get_http_headers())).
	WillOnce(Return(span));

	int* dummy = new int(2);
	void* reqHandle = dummy;
	auto res = engine.startRequest("ws_context", &payload, &reqHandle);
	EXPECT_EQ(res, OTEL_SUCCESS);

	auto* reqContext = (otel::core::RequestContext*)(reqHandle);
	ASSERT_TRUE(reqContext);
	EXPECT_TRUE(reqContext->rootSpan().get() != nullptr);
	EXPECT_EQ(reqContext->getContextName(), "ws_context");
	EXPECT_FALSE(reqContext->hasActiveInteraction());

  delete(dummy);
}


TEST(TestRequestProcessingEngine, StartRequestInvalidParams)
{
	FakeRequestProcessingEngine engine;
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	std::shared_ptr<otel::core::TenantConfig> config;
	engine.init(config, spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);
	std::string wscontext = "ws_context";
	otel::core::RequestPayload payload;

	auto res = engine.startRequest("ws_context", &payload, nullptr);
	EXPECT_EQ(res, OTEL_STATUS(handle_pointer_is_null));

	int* intPtr = new int(2);
	void* reqHandle = intPtr;
	res = engine.startRequest("ws_context", nullptr, &reqHandle);
	EXPECT_EQ(res, OTEL_STATUS(payload_reflector_is_null));
	delete(intPtr);
}


TEST(TestRequestProcessingEngine, EndRequest)
{
	FakeRequestProcessingEngine engine;
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	std::shared_ptr<otel::core::TenantConfig> config;
	engine.init(config, spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);
	std::shared_ptr<otel::core::sdkwrapper::IScopedSpan> rootSpan;
  	rootSpan.reset(new MockScopedSpan);
	auto* rContext = new otel::core::RequestContext(rootSpan);

	// adding some unfinished interactions
	std::shared_ptr<otel::core::sdkwrapper::IScopedSpan> interactionSpan1, interactionSpan2;
  	interactionSpan1.reset(new MockScopedSpan);
  	interactionSpan2.reset(new MockScopedSpan);
	rContext->addInteraction(interactionSpan1);
	rContext->addInteraction(interactionSpan2);

	auto getMockSpan = [](std::shared_ptr<otel::core::sdkwrapper::IScopedSpan> span) {
		return (MockScopedSpan*)(span.get());
	};

	// interactionSpans' End should be called in LIFO order followed by root span.
	EXPECT_CALL(*getMockSpan(interactionSpan2), End()).
	Times(1);

	EXPECT_CALL(*getMockSpan(interactionSpan1), End()).
	Times(1);

	unsigned int status_code = 403;
	EXPECT_CALL(*getMockSpan(rootSpan), SetStatus(otel::core::sdkwrapper::StatusCode::Error, "HTTP ERROR CODE:403")).Times(1);
	EXPECT_CALL(*getMockSpan(rootSpan),
		AddAttribute(kAttrHTTPStatusCode,
			HasUnsignedIntValue(status_code))).Times(1);
	
	EXPECT_CALL(*getMockSpan(rootSpan), End()).
	Times(1);

	std::unique_ptr<otel::core::ResponsePayload> responsePayload
        (new otel::core::ResponsePayload);
    responsePayload->status_code = status_code;
	auto res = engine.endRequest(rContext, "403", responsePayload.get());
	EXPECT_EQ(res, OTEL_SUCCESS);
}

TEST(TestRequestProcessingEngine, EndRequestInvalidParams)
{
	FakeRequestProcessingEngine engine;
	std::shared_ptr<otel::core::TenantConfig> config;
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	engine.init(config,spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);
	auto res = engine.endRequest(nullptr, "403");
	EXPECT_EQ(res, OTEL_STATUS(fail));
}


TEST(TestRequestProcessingEngine, StartInteraction)
{
	FakeRequestProcessingEngine engine;
	std::shared_ptr<otel::core::TenantConfig> config;
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	engine.init(config,spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);

	otel::core::InteractionPayload payload;
	payload.moduleName = "module";
	payload.phaseName = "phase";

	otel::core::sdkwrapper::OtelKeyValueMap keyValueMap;
	keyValueMap["interactionType"] = "EXIT_CALL";

	std::shared_ptr<otel::core::sdkwrapper::IScopedSpan> span;
  	span.reset(new MockScopedSpan);

	std::unordered_map<std::string, std::string> propagationHeaders;
	std::unordered_map<std::string, std::string> emptyHeaders;

	// sdkwrapper's create span function should be called
	using testing::_;
	EXPECT_CALL(*sdkWrapper, CreateSpan("module_phase",
		otel::core::sdkwrapper::SpanKind::CLIENT,
		HasMapVal(keyValueMap), emptyHeaders)).
	WillOnce(Return(span));

	// call to populatePropagationHeader of sdkWrapper
	EXPECT_CALL(*sdkWrapper, PopulatePropagationHeaders(propagationHeaders)).
	Times(1);

	std::shared_ptr<otel::core::sdkwrapper::IScopedSpan> rootSpan;
  	rootSpan.reset(new MockScopedSpan);
	auto* rContext = new otel::core::RequestContext(rootSpan);

	auto res = engine.startInteraction((void*)(rContext), &payload, propagationHeaders);
	EXPECT_EQ(res, OTEL_SUCCESS);

	// Interaction will be added in requestContext
	EXPECT_TRUE(rContext->hasActiveInteraction());
}

TEST(TestRequestProcessingEngine, StartInteractionInvalidParams)
{
	FakeRequestProcessingEngine engine;
	std::shared_ptr<otel::core::TenantConfig> config;
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	engine.init(config,spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);

	otel::core::InteractionPayload payload;
	std::unordered_map<std::string, std::string> propagationHeaders;

	// invalid request context
	auto res = engine.startInteraction(nullptr, &payload, propagationHeaders);
	EXPECT_EQ(res, OTEL_STATUS(handle_pointer_is_null));

	std::shared_ptr<otel::core::sdkwrapper::IScopedSpan> rootSpan;
  	rootSpan.reset(new MockScopedSpan);
	auto* rContext = new otel::core::RequestContext(rootSpan);

	// invalid payload
	res = engine.startInteraction((void*)(rContext), nullptr, propagationHeaders);
	EXPECT_EQ(res, OTEL_STATUS(payload_reflector_is_null));
}

TEST(TestRequestProcessingEngine, EndInteraction)
{
	FakeRequestProcessingEngine engine;
	std::shared_ptr<otel::core::TenantConfig> config;
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	engine.init(config,spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);

	std::unordered_map<std::string, std::string> propagationHeaders;
	otel::core::InteractionPayload startPayload;
	std::shared_ptr<otel::core::sdkwrapper::IScopedSpan> rootSpan;
  	rootSpan.reset(new MockScopedSpan);
	auto* rContext = new otel::core::RequestContext(rootSpan);
	std::shared_ptr<otel::core::sdkwrapper::IScopedSpan> interactionSpan;
  	interactionSpan.reset(new MockScopedSpan);
  	rContext->addInteraction(interactionSpan);

	otel::core::EndInteractionPayload payload;
	payload.errorCode = 403;
	payload.errorMsg = "error_msg";
	payload.backendName = "backend_one";
	payload.backendType = "backend_type";

	auto span = rContext->lastActiveInteraction();
  EXPECT_CALL(*(MockScopedSpan*)(span.get()), SetStatus(otel::core::sdkwrapper::StatusCode::Error, "error_msg")).
     Times(1);
  EXPECT_CALL(*(MockScopedSpan*)(span.get()), AddAttribute("error_code", HasLongIntValue(403))).
     Times(1);

  EXPECT_CALL(*(MockScopedSpan*)(span.get()), AddAttribute("backend_name", HasStringVal("backend_one"))).
                  Times(1);
  EXPECT_CALL(*(MockScopedSpan*)(span.get()), AddAttribute("backend_type", HasStringVal("backend_type"))).
                  Times(1);

	// interactionSpan's End should be called.
	EXPECT_CALL(*(MockScopedSpan*)(span.get()), End()).
	Times(1);

	auto res = engine.endInteraction(rContext, false, &payload);
	EXPECT_EQ(res, OTEL_SUCCESS);
}

TEST(TestRequestProcessingEngine, EndInteractionInvalidParams)
{
	FakeRequestProcessingEngine engine;
	std::shared_ptr<otel::core::TenantConfig> config;
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	engine.init(config, spanNamer);
	auto* sdkWrapper = engine.getMockSdkWrapper();
	ASSERT_TRUE(sdkWrapper);

	// invalid handle
	auto res = engine.endInteraction(nullptr, true, nullptr);
	EXPECT_EQ(res, OTEL_STATUS(handle_pointer_is_null));

	// invalid payload case
	std::shared_ptr<otel::core::sdkwrapper::IScopedSpan> rootSpan;
  	rootSpan.reset(new MockScopedSpan);
	auto* rContext = new otel::core::RequestContext(rootSpan);

	res = engine.endInteraction((void*)(rContext), true, nullptr);
	EXPECT_EQ(res, OTEL_STATUS(payload_reflector_is_null));

	// no interaction in the context
	otel::core::EndInteractionPayload payload;
	res = engine.endInteraction(rContext, true, &payload);
	EXPECT_EQ(res, OTEL_STATUS(invalid_context));
}

