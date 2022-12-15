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

#include <unordered_map>

std::shared_ptr<otel::core::TenantConfig> getConfig() {
  auto config =  std::shared_ptr<otel::core::TenantConfig>(new otel::core::TenantConfig());
  config->setServiceName("dummy_webserver");
  config->setServiceNamespace("dummy_service_namespace");
  config->setServiceInstanceId("dummy_instance_id");
  config->setOtelExporterType("otlp");
  config->setOtelExporterEndpoint("otel-collector:4317");
  config->setOtelLibraryName("opentelemetry-apache");
  return config;
}

TEST(IntegrationTest, StartRequest)
{
	otel::core::RequestProcessingEngine engine;
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
	auto config = getConfig();
	engine.init(config, spanNamer);
	std::string wscontext = "ws_context";
	otel::core::RequestPayload payload;
	payload.set_http_headers("key", "value");
	payload.set_uri("dummy_span");
	payload.set_request_protocol("GET");

	otel::core::sdkwrapper::OtelKeyValueMap keyValueMap;
  	keyValueMap["request_protocol"] = "GET";

  	std::shared_ptr<otel::core::sdkwrapper::IScopedSpan> span;
  	span.reset(new MockScopedSpan);

	int* dummy = new int(2);
	void* reqHandle = dummy;
  std::cout << "calling startReq" << std::endl;
  for (int i = 0; i < 20; i++) {
	  auto res = engine.startRequest("ws_context", &payload, &reqHandle);
	  EXPECT_EQ(res, OTEL_SUCCESS);
    otel::core::InteractionPayload iPayload;
    iPayload.moduleName = "module";
    iPayload.phaseName = "phase";

    otel::core::sdkwrapper::OtelKeyValueMap keyValueMap;
    keyValueMap["interactionType"] = "EXIT_CALL";


    std::unordered_map<std::string, std::string> propagationHeaders;
    std::unordered_map<std::string, std::string> emptyHeaders;
    engine.startInteraction(reqHandle, &iPayload, propagationHeaders);
    std::cout << "printing propagation headers" << std::endl;
    for (auto elem : propagationHeaders) {
        std::cout << elem.first << " " << elem.second << std::endl;
    }
    engine.endRequest(reqHandle, "error_msg");
  }
}

