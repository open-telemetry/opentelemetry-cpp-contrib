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
#include "sdkwrapper/ScopedSpan.h"
#include "sdkwrapper/ServerSpan.h"
#include "sdkwrapper/IScopedSpan.h"
#include "mocks/mock_SdkHelperFactory.h"
#include "sdkwrapper/SdkWrapper.h"
#include "mocks/mock_OpenTelemetry.h"
#include "mocks/mock_sdkwrapper.h"
#include <opentelemetry/common/key_value_iterable_view.h>

using ::testing::Return;
using namespace otel::core::sdkwrapper;
using::testing::_;
using ::testing::Args;

MATCHER_P(HasLongIntValue, value, "") {
	return nostd::get<int64_t>(arg) == value;
}

TEST(SdkWrapper, SdkWrapper_CreateSpan_Kind_Server)
{
	auto config = std::shared_ptr<otel::core::TenantConfig>(new otel::core::TenantConfig);
	FakeSdkWrapper sdkWrapper;
	sdkWrapper.Init(config);

	MockSdkHelperFactory *mockSdkHelperFactory =
		dynamic_cast<MockSdkHelperFactory*>(sdkWrapper.GetMockSdkHelperFactory());
	testing::Mock::AllowLeak(mockSdkHelperFactory);

	otel::core::OtelPropagators testOtelPropagators;
	testOtelPropagators.push_back(std::unique_ptr<MockPropagator>(
		new MockPropagator()));
	MockPropagator* mockPropagator = dynamic_cast<MockPropagator*>
		(testOtelPropagators[0].get());
	testing::Mock::AllowLeak(mockPropagator);

	nostd::shared_ptr<MockSpan>testOtelSpan(new MockSpan());
	nostd::shared_ptr<MockTracer>testTracer (new MockTracer());

	nostd::string_view testkey1 = "TestKey1";
	nostd::string_view testSpanName = "TestSpanName";
	auto kind = trace::SpanKind::kServer;
	auto spanKind = otel::core::sdkwrapper::SpanKind::SERVER;

	std::unordered_map<std::string, std::string> carrier {{"TestKey", "TestValue"}};
	int64_t testvalue = 100;
	std::unordered_map<std::string, common::AttributeValue>
		attributes{{"TestKey1", testvalue}};

	using ::testing::InvokeWithoutArgs;

	EXPECT_CALL(*mockPropagator, ExtractImpl(_)).Times(1);
	EXPECT_CALL(*mockSdkHelperFactory, GetPropagators()).
		Times(1).
		WillOnce(InvokeWithoutArgs([&]()-> otel::core::OtelPropagators&{
			return testOtelPropagators;
		}));

	EXPECT_CALL(*testTracer, VerifyKeyValueIterable(testkey1,
		HasLongIntValue(testvalue))).Times(1);
	EXPECT_CALL(*testTracer, StartSpanInternal(testSpanName, kind)).Times(1).
		WillOnce(InvokeWithoutArgs([&]()->nostd::shared_ptr<MockSpan>{
			return testOtelSpan;
		}));
	EXPECT_CALL(*mockSdkHelperFactory, GetTracer()).
		Times(1).
		WillRepeatedly(InvokeWithoutArgs([&]()->nostd::shared_ptr<MockTracer>{
			return testTracer;
		}));

	auto sdkSpan = sdkWrapper.CreateSpan("TestSpanName", spanKind, attributes, carrier);

	auto serverSpan = dynamic_cast<ServerSpan*>(sdkSpan.get());
	EXPECT_NE(nullptr, serverSpan);
}

TEST(SdkWrapper, SdkWrapper_CreateSpan_Kind_Client)
{
	auto config = std::shared_ptr<otel::core::TenantConfig>(new otel::core::TenantConfig);
	FakeSdkWrapper sdkWrapper;
	sdkWrapper.Init(config);

	MockSdkHelperFactory *mockSdkHelperFactory =
		dynamic_cast<MockSdkHelperFactory*>(sdkWrapper.GetMockSdkHelperFactory());
	testing::Mock::AllowLeak(mockSdkHelperFactory);

	otel::core::OtelPropagators testOtelPropagators;
	testOtelPropagators.push_back(std::unique_ptr<MockPropagator>(
		new MockPropagator()));
	MockPropagator* mockPropagator = dynamic_cast<MockPropagator*>
		(testOtelPropagators[0].get());
	testing::Mock::AllowLeak(mockPropagator);

	nostd::shared_ptr<MockSpan>testOtelSpan(new MockSpan());
	nostd::shared_ptr<MockTracer>testTracer (new MockTracer());

	nostd::string_view testkey1 = "TestKey1";
	nostd::string_view testSpanName = "TestSpanName";
	auto kind = trace::SpanKind::kClient;
	auto spanKind = otel::core::sdkwrapper::SpanKind::CLIENT;

	std::unordered_map<std::string, std::string> carrier {{"TestKey", "TestValue"}};
	int64_t testvalue = 100;
	std::unordered_map<std::string, common::AttributeValue>
		attributes{{"TestKey1", testvalue}};

	using ::testing::InvokeWithoutArgs;

	EXPECT_CALL(*testTracer, VerifyKeyValueIterable(testkey1,
		HasLongIntValue(testvalue))).Times(1);
	EXPECT_CALL(*testTracer, StartSpanInternal(testSpanName, kind)).Times(1).
		WillOnce(InvokeWithoutArgs([&]()->nostd::shared_ptr<MockSpan>{
			return testOtelSpan;
		}));
	EXPECT_CALL(*mockSdkHelperFactory, GetTracer()).
		Times(1).
		WillRepeatedly(InvokeWithoutArgs([&]()->nostd::shared_ptr<MockTracer>{
			return testTracer;
		}));

	auto sdkSpan = sdkWrapper.CreateSpan("TestSpanName", spanKind, attributes);

	auto scopedSpan = dynamic_cast<ScopedSpan*>(sdkSpan.get());
	EXPECT_NE(nullptr, scopedSpan);
}

TEST(SdkWrapper, SdkWrapper_PopulatePropagationHeaders)
{
	auto config = std::shared_ptr<otel::core::TenantConfig>(new otel::core::TenantConfig);
	FakeSdkWrapper sdkWrapper;
	sdkWrapper.Init(config);

	MockSdkHelperFactory *mockSdkHelperFactory =
		dynamic_cast<MockSdkHelperFactory*>(sdkWrapper.GetMockSdkHelperFactory());
	testing::Mock::AllowLeak(mockSdkHelperFactory);

	otel::core::OtelPropagators testOtelPropagators;
	testOtelPropagators.push_back(std::unique_ptr<MockPropagator>(
		new MockPropagator()));
	MockPropagator* mockPropagator = dynamic_cast<MockPropagator*>
		(testOtelPropagators[0].get());
	testing::Mock::AllowLeak(mockPropagator);

	nostd::shared_ptr<MockSpan>testOtelSpan(new MockSpan());
	nostd::shared_ptr<MockTracer>testTracer (new MockTracer());

	nostd::string_view testkey1 = "TestKey1";
	nostd::string_view testSpanName = "TestSpanName";
	auto kind = trace::SpanKind::kInternal;
	auto spanKind = otel::core::sdkwrapper::SpanKind::INTERNAL;

	std::unordered_map<std::string, std::string> carrier {{"TestKey", "TestValue"}};
	int64_t testvalue = 100;
	std::unordered_map<std::string, common::AttributeValue>
		attributes{{"TestKey1", testvalue}};

	using ::testing::InvokeWithoutArgs;
	EXPECT_CALL(*testTracer, VerifyKeyValueIterable(testkey1,
		HasLongIntValue(testvalue))).Times(1);
	EXPECT_CALL(*testTracer, StartSpanInternal(testSpanName, kind)).Times(1).
		WillOnce(InvokeWithoutArgs([&]()->nostd::shared_ptr<MockSpan>{
			return testOtelSpan;
		}));
	EXPECT_CALL(*mockSdkHelperFactory, GetTracer()).
		Times(1).
		WillRepeatedly(InvokeWithoutArgs([&]()->nostd::shared_ptr<MockTracer>{
			return testTracer;
		}));

	auto sdkSpan = sdkWrapper.CreateSpan("TestSpanName", spanKind, attributes);

	auto scopedSpan = dynamic_cast<ScopedSpan*>(sdkSpan.get());
	EXPECT_NE(nullptr, scopedSpan);

	using ::testing::Invoke;
	EXPECT_CALL(*mockSdkHelperFactory, GetPropagators()).
	  Times(1).
	  WillOnce(InvokeWithoutArgs([&]()-> otel::core::OtelPropagators&{
	    return testOtelPropagators;
	  }));
	EXPECT_CALL(*mockPropagator, InjectImpl(_)).Times(1);
	sdkWrapper.PopulatePropagationHeaders(carrier);
}
