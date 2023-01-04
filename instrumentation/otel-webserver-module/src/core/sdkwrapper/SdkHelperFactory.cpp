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

#include "sdkwrapper/SdkHelperFactory.h"
#include "sdkwrapper/SdkConstants.h"
#include "opentelemetry/sdk/trace/batch_span_processor.h"
#include "opentelemetry/sdk/trace/simple_processor.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/sdk/trace/samplers/always_on.h"
#include "opentelemetry/sdk/trace/samplers/always_off.h"
#include "opentelemetry/sdk/trace/samplers/parent.h"
#include "opentelemetry/sdk/trace/samplers/trace_id_ratio.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter.h"
#include "opentelemetry/exporters/otlp/otlp_environment.h"
#include "opentelemetry/baggage/propagation/baggage_propagator.h"
#include <module_version.h>
#include <fstream>
#include <iostream>

namespace otel {
namespace core {
namespace sdkwrapper {

// NOTE : all the validation checks related to otel inputs are done in the helper factory.
// So, these constants are not required outside of this execution unit
namespace {
  constexpr const char* OTLP_EXPORTER_TYPE = "otlp";
  constexpr const char* OSSTREAM_EXPORTER_TYPE = "osstream";
  constexpr const char* SIMPLE_PROCESSOR = "simple";
  constexpr const char* BATCH_PROCESSOR = "batch";
  constexpr const char* ALWAYS_ON_SAMPLER = "always_on";
  constexpr const char* ALWAYS_OFF_SAMPLER = "always_off";
  constexpr const char* PARENT_BASED_SAMPLER = "parent";
  constexpr const char* TRACE_ID_RATIO_BASED_SAMPLER = "trace_id_ratio";
}

SdkHelperFactory::SdkHelperFactory(
    std::shared_ptr<TenantConfig> config,
    const AgentLogger& logger) :
    mLogger(logger)
{
    // Create exporter, processor and provider.

    // TODO:: Use constant expressions
    LOG4CXX_INFO(mLogger, "ServiceNamespace: " << config->getServiceNamespace() <<
        " ServiceName: " << config->getServiceName() <<
        " ServiceInstanceId: " << config->getServiceInstanceId());
    sdk::resource::ResourceAttributes attributes;
    attributes[kServiceName] = config->getServiceName();
    attributes[kServiceNamespace] = config->getServiceNamespace();
    attributes[kServiceInstanceId] = config->getServiceInstanceId();

    // NOTE : resource attribute values are nostd::variant and so we need to explicitely set it to std::string
    std::string libraryVersion = MODULE_VERSION;
    std::string cppSDKVersion = CPP_SDK_VERSION;

    // NOTE : InstrumentationLibrary code is incomplete for the otlp exporter in sdk.
    // So, we need to pass libraryName and libraryVersion as resource attributes.
    // Ref : https://github.com/open-telemetry/opentelemetry-cpp/blob/main/exporters/otlp/src/otlp_recordable.cc
    
    //Library was changed to webengine to comply with specs https://github.com/open-telemetry/opentelemetry-specification/blob/main/specification/resource/semantic_conventions/webengine.md
    attributes[kOtelWebEngineName] = config->getOtelLibraryName();
    //attributes[kOtelWebEngineVersion] = libraryVersion;

    //attributes[kOtelWebEngineDescription] = config->getOtelLibraryName() + " Instrumentation";

    auto exporter = GetExporter(config);
    auto processor = GetSpanProcessor(config, std::move(exporter));
    auto sampler = GetSampler(config);

    mTracerProvider = opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider>(
      new opentelemetry::sdk::trace::TracerProvider(
            std::move(processor),
            opentelemetry::sdk::resource::Resource::Create(attributes),
            std::move(sampler)
            ));

    mTracer = mTracerProvider->GetTracer("cpp", cppSDKVersion);
    LOG4CXX_INFO(mLogger,
        "Tracer created with LibraryName: " << "cpp" <<
        " and LibraryVersion " << libraryVersion);

    // Adding trace propagator
    using MapHttpTraceCtx = opentelemetry::trace::propagation::HttpTraceContext;
    mPropagators.push_back(
        std::unique_ptr<MapHttpTraceCtx>(new MapHttpTraceCtx()));
}

OtelTracer SdkHelperFactory::GetTracer()
{
    return mTracer;
}

OtelPropagators& SdkHelperFactory::GetPropagators()
{
  return mPropagators;
}

OtelSpanExporter SdkHelperFactory::GetExporter(
    std::shared_ptr<TenantConfig> config)
{
    auto exporter = std::unique_ptr<opentelemetry::sdk::trace::SpanExporter>{};
    auto type = config->getOtelExporterType();

    if (type == OSSTREAM_EXPORTER_TYPE) {
        exporter.reset(new opentelemetry::exporter::trace::OStreamSpanExporter);
    } else {
        if (type != OTLP_EXPORTER_TYPE) {
          // default is otlp exporter
          LOG4CXX_WARN(mLogger, "Received unknown exporter type: " << type << ". Will create default(otlp) exporter");
          type = OTLP_EXPORTER_TYPE;
        }
        opentelemetry::exporter::otlp::OtlpGrpcExporterOptions opts;
        opts.endpoint = config->getOtelExporterEndpoint();
        if (config->getOtelSslEnabled()) {
            opts.use_ssl_credentials = config->getOtelSslEnabled();
            opts.ssl_credentials_cacert_path = config->getOtelSslCertPath();
            LOG4CXX_TRACE(mLogger, "Ssl Credentials are enabled for exporter, path: "
                << opts.ssl_credentials_cacert_path);
        }

        opentelemetry::common::KeyValueStringTokenizer tokenizer{config->getOtelExporterOtlpHeaders()};
        opentelemetry::nostd::string_view header_key;
        opentelemetry::nostd::string_view header_value;
        bool header_valid = true;
        std::unordered_set<std::string> remove_cache;

        while (tokenizer.next(header_valid, header_key, header_value))
        {
            if (header_valid)
            {
                std::string key = static_cast<std::string>(header_key);
                if (remove_cache.end() == remove_cache.find(key))
                {
                    remove_cache.insert(key);
                    auto range = opts.metadata.equal_range(key);
                    if (range.first != range.second)
                    {
                        opts.metadata.erase(range.first, range.second);
                    }
                }

                opts.metadata.emplace(std::make_pair(std::move(key), static_cast<std::string>(header_value)));
            }
        }

        exporter.reset(new opentelemetry::exporter::otlp::OtlpGrpcExporter(opts));
    }

    LOG4CXX_INFO(mLogger, "Exporter created with ExporterType: "
        << type);
    return std::move(exporter);
}

OtelSpanProcessor SdkHelperFactory::GetSpanProcessor(
    std::shared_ptr<TenantConfig> config,
    OtelSpanExporter exporter)
{
    auto processor = OtelSpanProcessor{};
    auto type = config->getOtelProcessorType();
    if (type == SIMPLE_PROCESSOR) {
        processor.reset(
                new opentelemetry::sdk::trace::SimpleSpanProcessor(std::move(exporter)));
    } else {
        if (type != BATCH_PROCESSOR) {
           // default is batch processor
           LOG4CXX_WARN(mLogger, "Received unknown processor type: " << type << ". Will create default(batch) processor");
           type = BATCH_PROCESSOR;
        }
        opentelemetry::sdk::trace::BatchSpanProcessorOptions options;
        processor.reset(
            new opentelemetry::sdk::trace::BatchSpanProcessor(std::move(exporter), options));
    }

    LOG4CXX_INFO(mLogger, "Processor created with ProcessorType: "
        << type);
    return processor;
}

OtelSampler SdkHelperFactory::GetSampler(
    std::shared_ptr<TenantConfig> config)
{
    auto sampler = OtelSampler{};
    auto type = config->getOtelSamplerType();

    if (type == ALWAYS_OFF_SAMPLER) {
        sampler.reset(new sdk::trace::AlwaysOffSampler);
    } else if (type == TRACE_ID_RATIO_BASED_SAMPLER) { // TODO
        ;
    } else if (type == PARENT_BASED_SAMPLER) { // TODO
        ;
    } else {
        if (type != ALWAYS_ON_SAMPLER) {
          // default is always_on sampler
          LOG4CXX_WARN(mLogger, "Received unknown sampler type: " << type << ". Will create default(always_on) sampler");
          type = ALWAYS_ON_SAMPLER;
        }
        sampler.reset(new sdk::trace::AlwaysOnSampler);
    }

    LOG4CXX_INFO(mLogger, "Sampler created with SamplerType : " <<
        type);
    return sampler;
}

} //sdkwrapper
} //core
} //otel

