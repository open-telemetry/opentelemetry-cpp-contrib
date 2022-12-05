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

#include "sdkwrapper/ScopedSpan.h"

namespace otel {
namespace core {
namespace sdkwrapper {

ScopedSpan::ScopedSpan(
	const std::string& name,
	const trace::SpanKind& kind,
	const OtelKeyValueMap& attributes,
	ISdkHelperFactory* sdkHelperFactory,
	const AgentLogger& logger) :
	mLogger(logger)
{
	trace::StartSpanOptions options{};
	options.kind = kind;
	mSpan = sdkHelperFactory->GetTracer()->StartSpan(name, attributes, options);
	mScope.reset(new trace::Scope(mSpan));
	mSpanKind = kind;
}

void ScopedSpan::End()
{
	mSpan->End();
}

void ScopedSpan::AddEvent(const std::string& name,
	const std::chrono::system_clock::time_point &timePoint,
    const OtelKeyValueMap& attributes)
{
	opentelemetry::common::SystemTimestamp timestamp(timePoint);
	mSpan->AddEvent(name, timestamp, attributes);
}

void ScopedSpan::AddAttribute(const std::string& key,
	const common::AttributeValue& value)
{
	mSpan->SetAttribute(key, value);
}

void ScopedSpan::SetStatus(const StatusCode status, const std::string& desc)
{
  trace::StatusCode otelStatus = trace::StatusCode::kUnset;
  switch(status) {
    case StatusCode::Error : {
      otelStatus = trace::StatusCode::kError;
    } break;
    case StatusCode::Ok : {
      otelStatus = trace::StatusCode::kOk;
    } break;
  }

  mSpan->SetStatus(otelStatus, desc);
}

SpanKind ScopedSpan::GetSpanKind()
{
	if (mSpanKind == trace::SpanKind::kServer)
		return SpanKind::SERVER;
	else if (mSpanKind == trace::SpanKind::kClient)
		return SpanKind::CLIENT;
	else
		return SpanKind::INTERNAL;
}

} //sdkwrapper
} //core
} //otel
