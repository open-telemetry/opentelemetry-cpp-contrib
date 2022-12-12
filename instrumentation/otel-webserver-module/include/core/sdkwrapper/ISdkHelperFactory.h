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

#ifndef __ISDKHELPERFACTORY_H
#define __ISDKHELPERFACTORY_H

#include <vector>
#include <unordered_map>
#include <opentelemetry/nostd/shared_ptr.h>
#include "opentelemetry/trace/tracer.h"
#include "opentelemetry/context/propagation/text_map_propagator.h"

namespace otel {
namespace core {

using OtelTracer = opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer>;
using OtelPropagators = std::vector<std::unique_ptr
	<opentelemetry::context::propagation::TextMapPropagator>>;

namespace sdkwrapper {

class ISdkHelperFactory {
public:
	virtual ~ISdkHelperFactory() = default;
	virtual OtelTracer GetTracer() = 0;
	virtual OtelPropagators& GetPropagators() = 0;
};

} // sdkwrapper
} // core
} // otel

#endif
