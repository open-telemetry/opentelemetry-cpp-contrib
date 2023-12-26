/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <log4cxx/consoleappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>
#include <log4cxx/xml/domconfigurator.h>

#include <opentelemetry/instrumentation/log4cxx/appender.h>

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

int main(int argc, char **argv)
{
  // Set up logger provider
  {
    using ProviderPtr = std::shared_ptr<logs_api::LoggerProvider>;
    auto exporter     = logs_exp::OStreamLogRecordExporterFactory::Create();
    auto processor    = logs_sdk::SimpleLogRecordProcessorFactory::Create(std::move(exporter));
    auto provider     = logs_sdk::LoggerProviderFactory::Create(std::move(processor));
    logs_api::Provider::SetLoggerProvider(ProviderPtr(provider.release()));
  }

  // Set up tracer provider
  {
    using ProviderPtr = std::shared_ptr<trace_api::TracerProvider>;
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
  auto root_logger = log4cxx::Logger::getRootLogger();
  auto otel_logger = log4cxx::Logger::getLogger("OTelLogger");

  const auto has_xml_config = argc > 1;
  const auto xml_config     = argv[1];

  if (has_xml_config)
  {
    std::cout << ">>> Setting up appender from " << xml_config << std::endl;
    log4cxx::xml::DOMConfigurator::configure(xml_config);
  }
  else
  {
    std::cout << ">>> Setting up appender programatically" << std::endl;
    const auto format     = "[%d{yyyy-MM-dd HH:mm:ss}] %c %-5p - %m%n";
    const auto layout_ptr = std::make_shared<log4cxx::PatternLayout>(format);
    root_logger->addAppender(std::make_shared<log4cxx::ConsoleAppender>(layout_ptr));
    otel_logger->addAppender(std::make_shared<log4cxx::OpenTelemetryAppender>());
    otel_logger->setAdditivity(false);
  }

  LOG4CXX_INFO(root_logger, "This message will be ignored");
  LOG4CXX_DEBUG(otel_logger, "This message will be processed");
}
