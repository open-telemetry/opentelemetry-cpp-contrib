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

#ifndef __SERVERSPAN_H
#define __SERVERSPAN_H

#include "sdkwrapper/IScopedSpan.h"
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/scope.h>
#include <unordered_map>
#include <opentelemetry/context/runtime_context.h>
#include "AgentLogger.h"

namespace otel {
namespace core {
namespace sdkwrapper {

class ScopedSpan;
class ISdkHelperFactory;

class ServerSpan : public IScopedSpan {
public:
	ServerSpan(const std::string& name,
        const OtelKeyValueMap& atttibutes,
        const std::unordered_map<std::string, std::string>& carrier,
        ISdkHelperFactory* sdkHelperFactory,
        const AgentLogger& logger);

	void End() override;

	void AddEvent(const std::string& name,
    	const std::chrono::system_clock::time_point &timePoint,
        const OtelKeyValueMap& attributes) override;

	void AddAttribute(const std::string& key,
        const SpanAttributeValue& value) override;

  void SetStatus(const StatusCode status, const std::string& desc) override;

  SpanKind GetSpanKind();

private:
	std::unique_ptr<ScopedSpan> mScopedSpan;
    nostd::unique_ptr<context::Token> mToken;
    const AgentLogger& mLogger;

};

} //sdkwrapper
} //core
} //otel

#endif
