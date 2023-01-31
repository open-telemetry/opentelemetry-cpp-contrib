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
#include "sdkwrapper/ServerSpan.h"
#include "sdkwrapper/ScopedSpan.h"
#include "mocks/mock_SdkHelperFactory.h"
#include "mocks/mock_OpenTelemetry.h"
#include <opentelemetry/common/key_value_iterable_view.h>

using ::testing::Return;
using namespace otel::core::sdkwrapper;
using ::testing::_;

MATCHER_P(HasLongIntValue, value, "") {
	return nostd::get<int64_t>(arg) == value;
}

TEST(ServerSpan, ServerSpan_Create)
{
	MockSdkHelperFactory mockSdkHelperFactory;
	otel::core::OtelPropagators testOtelPropagators;
	testOtelPropagators.push_back(std::unique_ptr<MockPropagator>(
		new MockPropagator()));
	MockPropagator* mockPropagator = dynamic_cast<MockPropagator*>
		(testOtelPropagators[0].get());
	testing::Mock::AllowLeak(mockPropagator);

	nostd::string_view testkey1 = "TestKey1";
	nostd::string_view testSpanName = "TestSpanName";
	auto kind = trace::SpanKind::kServer;

	std::unordered_map<std::string, std::string> carrier {{"TestKey", "TestValue"}};
	int64_t testvalue = 100;
	std::unordered_map<std::string, common::AttributeValue>
		attributes{{"TestKey1", testvalue}};

	nostd::shared_ptr<MockSpan>testOtelSpan(new MockSpan());
	nostd::shared_ptr<MockTracer>testTracer (new MockTracer());

	using ::testing::InvokeWithoutArgs;

	EXPECT_CALL(*mockPropagator, ExtractImpl(_)).Times(1);
	EXPECT_CALL(mockSdkHelperFactory, GetPropagators()).
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

	EXPECT_CALL(mockSdkHelperFactory, GetTracer()).
		Times(1).
		WillRepeatedly(InvokeWithoutArgs([&]()->nostd::shared_ptr<MockTracer>{
			return testTracer;
		}));

	AgentLogger logger = getLogger("Test");
	ServerSpan testSpan("TestSpanName", attributes, carrier, &mockSdkHelperFactory, logger);
}

TEST(ServerSpan, ServerSpan_End)
{
	MockSdkHelperFactory mockSdkHelperFactory;
	otel::core::OtelPropagators testOtelPropagators;
	testOtelPropagators.push_back(std::unique_ptr<MockPropagator>(
		new MockPropagator()));
	MockPropagator* mockPropagator = dynamic_cast<MockPropagator*>
		(testOtelPropagators[0].get());
	testing::Mock::AllowLeak(mockPropagator);

	nostd::string_view testkey1 = "TestKey1";
	nostd::string_view testSpanName = "TestSpanName";
	auto kind = trace::SpanKind::kServer;

	std::unordered_map<std::string, std::string> carrier {{"TestKey", "TestValue"}};
	int64_t testvalue = 100;
	std::unordered_map<std::string, common::AttributeValue>
		attributes{{"TestKey1", testvalue}};

	nostd::shared_ptr<MockSpan>testOtelSpan(new MockSpan());
	nostd::shared_ptr<MockTracer>testTracer (new MockTracer());

	using ::testing::InvokeWithoutArgs;

	EXPECT_CALL(*mockPropagator, ExtractImpl(_)).Times(1);
	EXPECT_CALL(mockSdkHelperFactory, GetPropagators()).
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

	EXPECT_CALL(mockSdkHelperFactory, GetTracer()).
		Times(1).
		WillRepeatedly(InvokeWithoutArgs([&]()->nostd::shared_ptr<MockTracer>{
			return testTracer;
		}));

	AgentLogger logger = getLogger("Test");
	ServerSpan testServerSpan("TestSpanName", attributes, carrier, &mockSdkHelperFactory, logger);
	using ::testing::_;
	EXPECT_CALL(*testOtelSpan, End(_)).Times(1);
	testServerSpan.End();
}

TEST(ServerSpan, ServerSpan_AddEvent)
{
	MockSdkHelperFactory mockSdkHelperFactory;
	otel::core::OtelPropagators testOtelPropagators;
	testOtelPropagators.push_back(std::unique_ptr<MockPropagator>(
		new MockPropagator()));
	MockPropagator* mockPropagator = dynamic_cast<MockPropagator*>
		(testOtelPropagators[0].get());
	testing::Mock::AllowLeak(mockPropagator);

	nostd::string_view testkey1 = "TestKey1";
	nostd::string_view testSpanName = "TestSpanName";
	auto kind = trace::SpanKind::kServer;

	std::unordered_map<std::string, std::string> carrier {{"TestKey", "TestValue"}};
	int64_t testvalue = 100;
	std::unordered_map<std::string, common::AttributeValue>
		attributes{{"TestKey1", testvalue}};

	nostd::shared_ptr<MockSpan>testOtelSpan(new MockSpan());
	nostd::shared_ptr<MockTracer>testTracer (new MockTracer());

	using ::testing::InvokeWithoutArgs;

	EXPECT_CALL(*mockPropagator, ExtractImpl(_)).Times(1);
	EXPECT_CALL(mockSdkHelperFactory, GetPropagators()).
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

	EXPECT_CALL(mockSdkHelperFactory, GetTracer()).
		Times(1).
		WillRepeatedly(InvokeWithoutArgs([&]()->nostd::shared_ptr<MockTracer>{
			return testTracer;
		}));

	AgentLogger logger = getLogger("Test");
	ServerSpan testServerSpan(
		"TestSpanName", attributes, carrier, &mockSdkHelperFactory, logger);

	std::string eventName {"TestEventName"};
	nostd::string_view eName {"TestEventName"};
	auto timePoint = std::chrono::system_clock::now();
	common::SystemTimestamp testTimeStamp(timePoint);

	EXPECT_CALL(*testOtelSpan, AddEvent(eName, testTimeStamp)).Times(1);
	EXPECT_CALL(*testOtelSpan, VerifyKeyValueIterable(testkey1,
		HasLongIntValue(testvalue))).Times(1);
	testServerSpan.AddEvent(eventName, timePoint, attributes);
}

TEST(ServerSpan, ServerSpan_AddAttribute)
{
	MockSdkHelperFactory mockSdkHelperFactory;
	otel::core::OtelPropagators testOtelPropagators;
	testOtelPropagators.push_back(std::unique_ptr<MockPropagator>(
		new MockPropagator()));
	MockPropagator* mockPropagator = dynamic_cast<MockPropagator*>
		(testOtelPropagators[0].get());
	testing::Mock::AllowLeak(mockPropagator);

	nostd::string_view testkey1 = "TestKey1";
	nostd::string_view testSpanName = "TestSpanName";
	auto kind = trace::SpanKind::kServer;

	std::unordered_map<std::string, std::string> carrier {{"TestKey", "TestValue"}};
	int64_t testvalue = 100;
	std::unordered_map<std::string, common::AttributeValue>
		attributes{{"TestKey1", testvalue}};

	nostd::shared_ptr<MockSpan>testOtelSpan(new MockSpan());
	nostd::shared_ptr<MockTracer>testTracer (new MockTracer());

	using ::testing::InvokeWithoutArgs;

	EXPECT_CALL(*mockPropagator, ExtractImpl(_)).Times(1);
	EXPECT_CALL(mockSdkHelperFactory, GetPropagators()).
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

	EXPECT_CALL(mockSdkHelperFactory, GetTracer()).
		Times(1).
		WillRepeatedly(InvokeWithoutArgs([&]()->nostd::shared_ptr<MockTracer>{
			return testTracer;
		}));

	AgentLogger logger = getLogger("Test");
	ServerSpan testServerSpan(
		"TestSpanName", attributes, carrier, &mockSdkHelperFactory, logger);

	EXPECT_CALL(*testOtelSpan, SetAttribute(testkey1,
		HasLongIntValue(testvalue))).Times(1);
	testServerSpan.AddAttribute("TestKey1", testvalue);
}
