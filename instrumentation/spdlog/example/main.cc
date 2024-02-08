/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <opentelemetry/instrumentation/spdlog/sink.h>

#include <opentelemetry/sdk/version/version.h>

#include <opentelemetry/exporters/ostream/log_record_exporter_factory.h>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/sdk/logs/exporter.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/processor.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>

#include <opentelemetry/exporters/ostream/span_exporter_factory.h>
#include <opentelemetry/sdk/trace/exporter.h>
#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/trace/provider.h>

namespace context   = opentelemetry::context;
namespace logs_exp  = opentelemetry::exporter::logs;
namespace logs_api  = opentelemetry::logs;
namespace logs_sdk  = opentelemetry::sdk::logs;
namespace trace_exp = opentelemetry::exporter::trace;
namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;

int main(int /* argc */, char ** /* argv */)
{
  // Set up logger provider
  {
    using ProviderPtr = opentelemetry::nostd::shared_ptr<logs_api::LoggerProvider>;
    auto exporter     = logs_exp::OStreamLogRecordExporterFactory::Create();
    auto processor    = logs_sdk::SimpleLogRecordProcessorFactory::Create(std::move(exporter));
    auto provider     = logs_sdk::LoggerProviderFactory::Create(std::move(processor));
    logs_api::Provider::SetLoggerProvider(ProviderPtr(provider.release()));
  }

  // Set up tracer provider
  {
    using ProviderPtr = opentelemetry::nostd::shared_ptr<trace_api::TracerProvider>;
    auto exporter     = trace_exp::OStreamSpanExporterFactory::Create();
    auto processor    = trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
    auto provider     = trace_sdk::TracerProviderFactory::Create(std::move(processor));
    trace_api::Provider::SetTracerProvider(ProviderPtr(provider.release()));
  }

  // Set up trace, span and context
  auto tracer  = trace_api::Provider::GetTracerProvider()->GetTracer("log4cxx_library",
                                                                     OPENTELEMETRY_SDK_VERSION);
  auto span    = tracer->StartSpan("log4cxx test span");
  auto ctx     = context::RuntimeContext::GetCurrent();
  auto new_ctx = ctx.SetValue("active_span", span);
  auto token   = context::RuntimeContext::Attach(new_ctx);

  // Set up loggers
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::debug);
  auto otel_sink = std::make_shared<spdlog::sinks::opentelemetry_sink_mt>();
  otel_sink->set_level(spdlog::level::info);

  auto logger = spdlog::logger("OTelLogger", {console_sink, otel_sink});
  logger.set_level(spdlog::level::debug);

  SPDLOG_LOGGER_DEBUG(&logger, "This message will be ignored");
  SPDLOG_LOGGER_INFO(&logger, "This message will be processed");
}
