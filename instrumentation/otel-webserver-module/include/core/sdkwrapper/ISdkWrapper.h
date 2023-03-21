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

#ifndef __ISDKWRAPPER_H
#define __ISDKWRAPPER_H

#include <unordered_map>
#include <memory>
#include "sdkwrapper/SdkEnums.h"
#include "sdkwrapper/IScopedSpan.h"
#include "api/TenantConfig.h"

namespace otel {
namespace core {
namespace sdkwrapper {

class ISdkWrapper {
public:
	virtual ~ISdkWrapper() = default;

	virtual void Init(std::shared_ptr<TenantConfig> config) = 0;

	virtual std::shared_ptr<IScopedSpan> CreateSpan(
		const std::string& name,
		const SpanKind& kind,
		const OtelKeyValueMap& attributes,
		const std::unordered_map<std::string, std::string>& carrier = {}) = 0;

	virtual void PopulatePropagationHeaders(
		std::unordered_map<std::string, std::string>& carrier) = 0;
};

} //sdkwrapper
} //core
} //otel


#endif
