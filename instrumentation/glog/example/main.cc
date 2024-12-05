/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <glog/logging.h>

#include <opentelemetry/instrumentation/glog/sink.h>

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

#include <cstdlib>
#include <fstream>
#include <iostream>

namespace context   = opentelemetry::context;
namespace logs_exp  = opentelemetry::exporter::logs;
namespace logs_api  = opentelemetry::logs;
namespace logs_sdk  = opentelemetry::sdk::logs;
namespace trace_exp = opentelemetry::exporter::trace;
namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;

int main(int /* argc */, char **argv)
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
  auto tracer  = trace_api::Provider::GetTracerProvider()->GetTracer("glog_library",
                                                                     OPENTELEMETRY_SDK_VERSION);
  auto span    = tracer->StartSpan("glog test span");
  auto ctx     = context::RuntimeContext::GetCurrent();
  auto new_ctx = ctx.SetValue("active_span", span);
  auto token   = context::RuntimeContext::Attach(new_ctx);

  // Set up loggers
  const auto log_file = std::string(argv[0]).append(".INFO");
  setenv("GLOG_minloglevel", "0", 1);
  google::InitGoogleLogging(argv[0]);
  google::SetLogDestination(google::INFO, log_file.c_str());

  LOG(INFO) << "This message will be ignored and logged by default.";
  LOG_TO_SINK/*_AND_TO_LOGFILE*/(nullptr, INFO) << "This message will be ignored but logged.";
  LOG_TO_SINK_BUT_NOT_TO_LOGFILE(nullptr, INFO) << "This message will be ignored and not logged.";
  google::OpenTelemetrySink sink;
  LOG_TO_SINK/*_AND_TO_LOGFILE*/(&sink, INFO) << "This message will be processed and logged.";
  LOG_TO_SINK_BUT_NOT_TO_LOGFILE(&sink, INFO) << "This message will be processed but not logged.";

  google::ShutdownGoogleLogging();
  
  // Display contents of log file
  std::ifstream fin(log_file, std::ios::binary);

  if(fin.is_open())
  {
    std::cout << "==============================" << std::endl;
    std::cout << "Contents of log file " << log_file << ":" << std::endl;
    std::cout << fin.rdbuf() << std::endl;
    std::cout << "==============================" << std::endl;
  }

  return 0;
}
