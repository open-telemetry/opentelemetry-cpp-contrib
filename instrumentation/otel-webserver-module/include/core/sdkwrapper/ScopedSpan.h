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

#ifndef __SCOPEDSPAN_H
#define __SCOPEDSPAN_H

#include "sdkwrapper/IScopedSpan.h"
#include "sdkwrapper/ISdkHelperFactory.h"
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/nostd/unique_ptr.h>
#include "AgentLogger.h"

namespace otel {
namespace core {
namespace sdkwrapper {

class ScopedSpan : public IScopedSpan {
public:
	ScopedSpan(const std::string& name,
        const trace::SpanKind& kind,
     	const OtelKeyValueMap& attributes,
        ISdkHelperFactory* sdkHelperFactory,
        const AgentLogger& logger);

	void End() override;

	void AddEvent(const std::string& name,
        const std::chrono::system_clock::time_point &time_point,
        const OtelKeyValueMap& attributes) override;

	void AddAttribute(const std::string& key,
        const SpanAttributeValue& value) override;

    void SetStatus(const StatusCode status, const std::string& desc) override;

    SpanKind GetSpanKind();

private:
	opentelemetry::nostd::shared_ptr<trace::Span> mSpan;
	std::unique_ptr<trace::Scope> mScope;
    const AgentLogger& mLogger;
    trace::SpanKind mSpanKind;
};

} //sdkwrapper
} //core
} //otel

#endif
