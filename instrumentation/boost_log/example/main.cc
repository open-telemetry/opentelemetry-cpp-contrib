/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>

#include <opentelemetry/instrumentation/boost_log/sink.h>

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
  auto tracer  = trace_api::Provider::GetTracerProvider()->GetTracer("boost_library",
                                                                     OPENTELEMETRY_SDK_VERSION);
  auto span    = tracer->StartSpan("boost log test span");
  auto ctx     = context::RuntimeContext::GetCurrent();
  auto new_ctx = ctx.SetValue("active_span", span);
  auto token   = context::RuntimeContext::Attach(new_ctx);

  // Set up loggers
  {
    using boost::log::sinks::synchronous_sink;
    using opentelemetry::instrumentation::boost_log::OpenTelemetrySinkBackend;
    auto backend = boost::make_shared<OpenTelemetrySinkBackend>();
    auto sink    = boost::make_shared<synchronous_sink<OpenTelemetrySinkBackend>>(backend);
    boost::log::core::get()->add_sink(sink);
    boost::log::add_common_attributes();
  }

  boost::log::sources::logger simple;
  BOOST_LOG(simple) << "Test simplest message";

  boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger;
  BOOST_LOG_SEV(logger, boost::log::trivial::info) << "Test message with severity";

  BOOST_LOG_SEV(logger, boost::log::trivial::debug)
      << boost::log::add_value("FileName", __FILE__)
      << boost::log::add_value("FunctionName", __FUNCTION__)
      << boost::log::add_value("LineNumber", __LINE__) << "Test message with source location";
}
