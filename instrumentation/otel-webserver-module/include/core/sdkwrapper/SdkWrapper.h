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

#ifndef __SDKWRAPPER_H
#define __SDKWRAPPER_H

#include "sdkwrapper/ISdkWrapper.h"
#include "sdkwrapper/ISdkHelperFactory.h"
#include "AgentLogger.h"
#include <memory>

namespace otel {
namespace core {
namespace sdkwrapper {

class SdkWrapper : public ISdkWrapper{
public:
	SdkWrapper();

	void Init(std::shared_ptr<TenantConfig> config) override;

	std::shared_ptr<IScopedSpan> CreateSpan(
		const std::string& name,
		const SpanKind& kind,
		const OtelKeyValueMap& attributes,
		const std::unordered_map<std::string, std::string>& carrier = {}) override;

	void PopulatePropagationHeaders(
		std::unordered_map<std::string, std::string>& carrier) override;

private:
	trace::SpanKind GetTraceSpanKind(const SpanKind& kind);

protected:
	std::unique_ptr<ISdkHelperFactory> mSdkHelperFactory;
	AgentLogger mLogger;
};

} //sdkwrapper
} //core
} //otel

#endif
