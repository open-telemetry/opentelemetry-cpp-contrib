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
#include "gtest/gtest.h"
#include "sdkwrapper/SdkUtils.h"

TEST(Get, ReturnsExpectedValueWhenKeyFound)
{
	std::unordered_map<std::string, std::string> carrier
		{{"TestKey", "TestValue"}};
	opentelemetry::nostd::string_view key = "TestKey";
	otel::core::sdkwrapper::OtelCarrier otelCarrier(carrier);
	auto value = otelCarrier.Get(key);
	EXPECT_EQ(value, "TestValue");
}

TEST(Get, ReturnsEmptyWhenKeyNotFound)
{
	std::unordered_map<std::string, std::string> carrier
		{{"TestKey", "TestValue"}};
	opentelemetry::nostd::string_view key = "TestKey1";
	otel::core::sdkwrapper::OtelCarrier otelCarrier(carrier);
	auto value = otelCarrier.Get(key);
	EXPECT_EQ(value, "");
}

TEST(Set, InsertsExpectedKeyAndValue)
{
	std::unordered_map<std::string, std::string> carrier;
	opentelemetry::nostd::string_view key = "TestKey";
	opentelemetry::nostd::string_view value = "TestValue";
	otel::core::sdkwrapper::OtelCarrier otelCarrier;
	otelCarrier.Set(key, value);
	EXPECT_EQ(otelCarrier.Get(key), value);
}

