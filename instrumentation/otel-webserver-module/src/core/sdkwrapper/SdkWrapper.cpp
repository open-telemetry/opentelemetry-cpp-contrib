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

constexpr const char* CARRIER_HEADER_NAME[] = {"X-B3-TraceId", "X-B3-SpanId", "X-B3-Sampled", "traceparent", "tracestate"};
const size_t CARRIER_HEADER_LEN = sizeof(CARRIER_HEADER_NAME)/sizeof(CARRIER_HEADER_NAME[0]);

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
std::string SdkWrapper::ReturnCurrentSpanId(){
	
	auto context = context::RuntimeContext::GetCurrent();
	auto currentSpan = trace::GetSpan(context);
	trace::SpanContext spanContext = currentSpan->GetContext();
	trace::SpanId spanId = spanContext.span_id();
	constexpr int len = 2 * trace::SpanId::kSize;
	char* data = new char[len];
	spanId.ToLowerBase16(nostd::span<char, len>{data, len});
	std::string currentSpanId(data, len);
	delete[] data;
	return currentSpanId;
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
	for (int i = 0; i < CARRIER_HEADER_LEN; i++) {
		auto carrier_header = otelCarrier.Get(CARRIER_HEADER_NAME[i]).data();
		if(carrier_header != ""){
			carrier[CARRIER_HEADER_NAME[i]] = otelCarrier.Get(CARRIER_HEADER_NAME[i]).data();
		}
	}
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
