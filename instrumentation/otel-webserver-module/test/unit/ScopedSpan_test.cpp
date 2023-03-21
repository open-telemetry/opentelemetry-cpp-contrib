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
#include "mocks/mock_SdkHelperFactory.h"
#include "mocks/mock_OpenTelemetry.h"
#include <opentelemetry/common/key_value_iterable_view.h>
#include "AgentLogger.h"
#include <log4cxx/logger.h>
#include <log4cxx/rolling/rollingfileappenderskeleton.h>
#include <log4cxx/xml/domconfigurator.h>
#include <boost/lexical_cast.hpp>

using ::testing::Return;
using namespace otel::core::sdkwrapper;

const std::string kTestSpanName{"TestSpanName"};

nostd::shared_ptr<MockTracer> ActionGetTracer()
{
	static nostd::shared_ptr<MockTracer>testTracer (new MockTracer());
	return testTracer;
}

nostd::shared_ptr<MockSpan> ActionGetSpans()
{
	static nostd::shared_ptr<MockSpan>testOtelSpan(new MockSpan());
	return testOtelSpan;
}

MATCHER_P(HasStringValue, value, "") {
	return nostd::get<nostd::string_view>(arg) == value;
}

MATCHER_P(HasLongIntValue, value, "") {
	return nostd::get<int64_t>(arg) == value;
}

TEST(ScopedSpan, ScopedSpan_Create)
{
	MockSdkHelperFactory mockSdkHelperFactory;

	using ::testing::InvokeWithoutArgs;
	auto testTracer = ActionGetTracer();

	nostd::string_view testkey1 = "TestKey1";
	int64_t testvalue = 100;
	std::unordered_map<std::string, common::AttributeValue>
		attributes{{"TestKey1", testvalue}};

	auto kind = trace::SpanKind::kInternal;
	nostd::string_view testSpanName = "TestSpanName";
	EXPECT_CALL(*testTracer, VerifyKeyValueIterable(testkey1,
		HasLongIntValue(testvalue))).Times(1);
	EXPECT_CALL(*testTracer, StartSpanInternal(testSpanName, kind)).Times(1).
		WillOnce(InvokeWithoutArgs(ActionGetSpans));

	EXPECT_CALL(mockSdkHelperFactory, GetTracer()).
		Times(1).
		WillRepeatedly(InvokeWithoutArgs(ActionGetTracer));

	AgentLogger logger = getLogger("Test");
	ScopedSpan testSpan(kTestSpanName,
		opentelemetry::trace::SpanKind::kInternal,
		attributes,
		&mockSdkHelperFactory,
		logger);
}

TEST(ScopedSpan, ScopedSpan_End)
{
	MockSdkHelperFactory mockSdkHelperFactory;
	auto testTracer = ActionGetTracer();
	auto testSpan = ActionGetSpans();

	using ::testing::InvokeWithoutArgs;
	using ::testing::_;

	EXPECT_CALL(*testTracer, StartSpanInternal(_, _)).Times(1).
		WillOnce(InvokeWithoutArgs(ActionGetSpans));

	EXPECT_CALL(mockSdkHelperFactory, GetTracer()).
		Times(1).
		WillRepeatedly(InvokeWithoutArgs(ActionGetTracer));
	AgentLogger logger = getLogger("Test");
	ScopedSpan testScopedSpan(kTestSpanName,
		opentelemetry::trace::SpanKind::kInternal,
		OtelKeyValueMap{},
		&mockSdkHelperFactory,
		logger);


	EXPECT_CALL(*testSpan, End(_)).Times(1);
	testScopedSpan.End();
}

TEST(ScopedSpan, ScopedSpan_AddEvent)
{
	MockSdkHelperFactory mockSdkHelperFactory;
	auto testTracer = ActionGetTracer();
	auto testSpan = ActionGetSpans();

	using ::testing::InvokeWithoutArgs;
	using ::testing::_;
	using ::testing::WhenDynamicCastTo;

	EXPECT_CALL(*testTracer, StartSpanInternal(_, _)).Times(1).
		WillOnce(InvokeWithoutArgs(ActionGetSpans));

	EXPECT_CALL(mockSdkHelperFactory, GetTracer()).
		Times(1).
		WillRepeatedly(InvokeWithoutArgs(ActionGetTracer));

	AgentLogger logger = getLogger("Test");
	ScopedSpan testScopedSpan(kTestSpanName,
		opentelemetry::trace::SpanKind::kInternal,
		OtelKeyValueMap{},
		&mockSdkHelperFactory,
		logger);


	std::string eventName {"TestEventName"};
	nostd::string_view eName {"TestEventName"};
	auto timePoint = std::chrono::system_clock::now();
	common::SystemTimestamp testTimeStamp(timePoint);
	std::unordered_map<std::string, common::AttributeValue> emptyAttributes;

	EXPECT_CALL(*testSpan, AddEvent(eName, testTimeStamp)).Times(1);
	testScopedSpan.AddEvent(eventName, timePoint, emptyAttributes);
}

TEST(ScopedSpan, ScopedSpan_AddAttribute)
{
	MockSdkHelperFactory mockSdkHelperFactory;
	auto testTracer = ActionGetTracer();
	auto testSpan = ActionGetSpans();

	using ::testing::InvokeWithoutArgs;
	using ::testing::_;

	EXPECT_CALL(*testTracer, StartSpanInternal(_, _)).Times(1).
		WillOnce(InvokeWithoutArgs(ActionGetSpans));

	EXPECT_CALL(mockSdkHelperFactory, GetTracer()).
		Times(1).
		WillRepeatedly(InvokeWithoutArgs(ActionGetTracer));

	AgentLogger logger = getLogger("Test");
	ScopedSpan testScopedSpan(kTestSpanName,
		opentelemetry::trace::SpanKind::kInternal,
		OtelKeyValueMap{},
		&mockSdkHelperFactory,
		logger);

	nostd::string_view testkey("TestKey");
	nostd::string_view testValueStr = "TestValue";
	common::AttributeValue testvalue(testValueStr);
	EXPECT_CALL(*testSpan, SetAttribute(testkey,
		HasStringValue("TestValue"))).Times(1);
	testScopedSpan.AddAttribute("TestKey", testvalue);
}
