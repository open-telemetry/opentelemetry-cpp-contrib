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

#include "sdkwrapper/ServerSpan.h"
#include "sdkwrapper/ScopedSpan.h"
#include "sdkwrapper/SdkUtils.h"
namespace otel {
namespace core {
namespace sdkwrapper {

ServerSpan::ServerSpan(const std::string& name,
	const OtelKeyValueMap& attributes,
	const std::unordered_map<std::string, std::string>& carrier,
	ISdkHelperFactory* sdkHelperFactory,
	const AgentLogger& logger) :
	mLogger(logger)
{
	// Extract W3C Trace Context. And store the token in local variable.
	context::Context ctx = context::RuntimeContext::GetCurrent();
	OtelCarrier otelCarrier(carrier);
	for (auto &propagator : sdkHelperFactory->GetPropagators()) {
		ctx = propagator->Extract(otelCarrier, ctx);
	}
	mToken = context::RuntimeContext::Attach(ctx);

	mScopedSpan.reset(
		new ScopedSpan(name,
			trace::SpanKind::kServer,
			attributes,
			sdkHelperFactory,
			mLogger));
}

void ServerSpan::End()
{
	context::RuntimeContext::Detach(*mToken.get());
	mScopedSpan->End();
}

void ServerSpan::AddEvent(const std::string& name,
	const std::chrono::system_clock::time_point &timePoint,
    const OtelKeyValueMap& attributes)
{
	mScopedSpan->AddEvent(name, timePoint, attributes);
}

void ServerSpan::AddAttribute(const std::string& key,
	const common::AttributeValue& value)
{
	mScopedSpan->AddAttribute(key, value);
}

void ServerSpan::SetStatus(const StatusCode status, const std::string& desc)
{
  mScopedSpan->SetStatus(status, desc);
}

SpanKind ServerSpan::GetSpanKind()
{
  return SpanKind::SERVER;
}


} //sdkwrapper
} //core
} //otel
