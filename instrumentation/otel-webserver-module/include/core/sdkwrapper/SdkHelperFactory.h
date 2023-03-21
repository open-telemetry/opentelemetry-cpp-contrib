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

#ifndef __SDKHELPERFACTORY_H
#define __SDKHELPERFACTORY_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/exporters/ostream/span_exporter.h>
#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/resource/resource.h>
#include "sdkwrapper/ISdkHelperFactory.h"
#include <opentelemetry/sdk/trace/sampler.h>
#include "opentelemetry/trace/tracer_provider.h"
#include "sdkwrapper/ISdkWrapper.h"
#include "AgentLogger.h"

namespace otel {
namespace core {
namespace sdkwrapper {

using namespace opentelemetry;
using OtelSpanExporter = std::unique_ptr<opentelemetry::sdk::trace::SpanExporter>;
using OtelSpanProcessor = std::unique_ptr<opentelemetry::sdk::trace::SpanProcessor>;
using OtelSampler = std::unique_ptr<opentelemetry::sdk::trace::Sampler>;
using OtelTracerProvider = opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider>;

class SdkHelperFactory: public ISdkHelperFactory {
public:
	SdkHelperFactory(
		std::shared_ptr<TenantConfig> config,
		const AgentLogger& logger);

	OtelTracer GetTracer() override;

	OtelPropagators& GetPropagators() override;

private:
	OtelSpanExporter GetExporter(std::shared_ptr<TenantConfig> config);

	OtelSpanProcessor GetSpanProcessor(std::shared_ptr<TenantConfig> config, OtelSpanExporter exporter);

	OtelSampler GetSampler(std::shared_ptr<TenantConfig> config);

    // There is one-to-one mapping of WebserverContext(Virtual Host) to TracerProvider.
    // Therefore, tracer provided can't be stored as global variable and needs to be accessed
    // per WebserverContext.
    OtelTracerProvider mTracerProvider;
	OtelTracer mTracer;
	OtelPropagators mPropagators;
	const AgentLogger& mLogger;
};

} //sdkwrapper
} //core
} //otel

#endif
