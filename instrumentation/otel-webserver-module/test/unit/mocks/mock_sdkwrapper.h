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
#include <gmock/gmock.h>
#include "sdkwrapper/ISdkWrapper.h"
#include "sdkwrapper/IScopedSpan.h"
#include "sdkwrapper/SdkWrapper.h"
#include "mock_SdkHelperFactory.h"

class MockScopedSpan : public otel::core::sdkwrapper::IScopedSpan
{
public:
	MOCK_METHOD(void,  End,
		(),
        (override));

    MOCK_METHOD(void, AddEvent,
        (const std::string& name,
        const std::chrono::system_clock::time_point &timePoint,
        const otel::core::sdkwrapper::OtelKeyValueMap& attributes),
        (override));

    MOCK_METHOD(void, AddAttribute,
        (const std::string& key,
        const otel::core::sdkwrapper::SpanAttributeValue& value),
        (override, noexcept));

    MOCK_METHOD(void, SetStatus,
       (const otel::core::sdkwrapper::StatusCode code,
        const std::string& desc),
       (override));

    MOCK_METHOD(otel::core::sdkwrapper::SpanKind,  GetSpanKind,
        (),
        (override));
};

// TODO : General MOCK_METHOD command is giving some unexpected errors. Revisit later
class MockSdkWrapper : public otel::core::sdkwrapper::ISdkWrapper
{
public:
    void Init(std::shared_ptr<otel::core::TenantConfig>) override {}
    MOCK_METHOD1(PopulatePropagationHeaders, void(std::unordered_map<std::string, std::string>& carrier));
    MOCK_METHOD4(CreateSpan, std::shared_ptr<otel::core::sdkwrapper::IScopedSpan>(
        const std::string& name,
        const otel::core::sdkwrapper::SpanKind& kind,
        const otel::core::sdkwrapper::OtelKeyValueMap& attributes,
        const std::unordered_map<std::string, std::string>& carrier));
};

class FakeSdkWrapper : public otel::core::sdkwrapper::SdkWrapper
{
public:
	void Init(std::shared_ptr<otel::core::TenantConfig> config)
	{
		mSdkHelperFactory = std::unique_ptr<MockSdkHelperFactory>(new MockSdkHelperFactory);
	}

	otel::core::sdkwrapper::ISdkHelperFactory* GetMockSdkHelperFactory()
	{
		return mSdkHelperFactory.get();
	}
};
