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

#include "sdkwrapper/SdkWrapper.h"
#include "sdkwrapper/ServerSpan.h"
#include "sdkwrapper/ScopedSpan.h"
#include "sdkwrapper/SdkUtils.h"
#include "sdkwrapper/SdkHelperFactory.h"

namespace otel {
namespace core {
namespace sdkwrapper {

namespace {

constexpr const char* TRACEPARENT_HEADER_NAME = "traceparent";
constexpr const char* TRACESTATE_HEADER_NAME = "tracestate";

} // anyonymous

SdkWrapper::SdkWrapper() :
	mLogger(getLogger(std::string(LogContext::AGENT) + ".SdkWrapper"))
{}

void SdkWrapper::Init(std::shared_ptr<TenantConfig> config)
{
	mSdkHelperFactory = std::unique_ptr<ISdkHelperFactory>(
		new SdkHelperFactory(config, mLogger));
}

std::shared_ptr<IScopedSpan> SdkWrapper::CreateSpan(
	const std::string& name,
	const SpanKind& kind,
	const OtelKeyValueMap& attributes,
	const std::unordered_map<std::string, std::string>& carrier)
{
	LOG4CXX_DEBUG(mLogger, "Creating Span of kind: "
		<< static_cast<int>(kind));
	trace::SpanKind traceSpanKind = GetTraceSpanKind(kind);
	if (traceSpanKind == trace::SpanKind::kServer) {
		return std::shared_ptr<IScopedSpan>(new ServerSpan(
			name,
			attributes,
			carrier,
			mSdkHelperFactory.get(),
			mLogger));
	} else {
		return std::shared_ptr<IScopedSpan>(new ScopedSpan(
			name,
			traceSpanKind,
			attributes,
			mSdkHelperFactory.get(),
			mLogger));
	}
}

void SdkWrapper::PopulatePropagationHeaders(
	std::unordered_map<std::string, std::string>& carrier) {

  // TODO : This is inefficient change as we are copying otel carrier data
  // into unordered map and sending it back to agent.
  // Ideally agent should keep otelCarrier data structure on its side.
  auto otelCarrier = OtelCarrier();
	auto context = context::RuntimeContext::GetCurrent();
	for (auto &propagators : mSdkHelperFactory->GetPropagators()) {
		propagators->Inject(otelCarrier, context);
	}

  // copy all relevant kv pairs into carrier
  carrier[TRACEPARENT_HEADER_NAME] = otelCarrier.Get(TRACEPARENT_HEADER_NAME).data();
  carrier[TRACESTATE_HEADER_NAME] = otelCarrier.Get(TRACESTATE_HEADER_NAME).data();
}

trace::SpanKind SdkWrapper::GetTraceSpanKind(const SpanKind& kind)
{
	trace::SpanKind traceSpanKind = trace::SpanKind::kInternal;
	switch(kind) {
		case SpanKind::INTERNAL:
			traceSpanKind = trace::SpanKind::kInternal;
			break;
		case SpanKind::SERVER:
			traceSpanKind = trace::SpanKind::kServer;
			break;
		case SpanKind::CLIENT:
			traceSpanKind = trace::SpanKind::kClient;
			break;
	}
	return traceSpanKind;
}

} //sdkwrapper
} //core
} //otel
