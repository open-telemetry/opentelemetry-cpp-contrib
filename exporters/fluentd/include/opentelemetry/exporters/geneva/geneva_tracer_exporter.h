// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#define USE_UNIX_SOCKETS
#include "geneva_exporter_options.h"
#include <opentelemetry/exporters/fluentd/common/socket_tools.h>
#include <opentelemetry/exporters/fluentd/trace/fluentd_exporter.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>
#include <opentelemetry/sdk/trace/batch_span_processor_options.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/trace/provider.h>

OPENTELEMETRY_BEGIN_NAMESPACE

namespace exporters {

namespace geneva {

class GenevaTracerExporter  {

public:
static inline bool InitializeGenevaExporter( const GenevaExporterOptions options ) {
    // Use only unix-domain IPC for agent connectivity
    if (options.socket_endpoint.size() > kUnixDomainSchemeLength  && options.socket_endpoint.substr(0, kUnixDomainSchemeLength) == std::string(kUnixDomainScheme)){
        opentelemetry::exporter::fluentd::common::FluentdExporterOptions  fluentd_options;
        opentelemetry::sdk::trace::BatchSpanProcessorOptions batch_processor_options;
        fluentd_options.retry_count = options.retry_count;
        fluentd_options.endpoint = options.socket_endpoint;
        batch_processor_options.max_queue_size = options.max_queue_size;
        batch_processor_options.schedule_delay_millis = options.schedule_delay_millis;
        batch_processor_options.max_export_batch_size = options.max_export_batch_size;
        auto exporter = std::unique_ptr<opentelemetry::sdk::trace::SpanExporter>(
            new opentelemetry::exporter::fluentd::trace::FluentdExporter(fluentd_options));
        auto processor = std::unique_ptr<opentelemetry::sdk::trace::SpanProcessor>(
            new opentelemetry::sdk::trace::BatchSpanProcessor(std::move(exporter), batch_processor_options));
        auto provider = opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider>(
            new opentelemetry::sdk::trace::TracerProvider(std::move(processor)));
        opentelemetry::trace::Provider::SetTracerProvider(provider);
        return true;
    } else {
#if defined(__EXCEPTIONS)
        throw new std::runtime_error("Invalid endpoint! Unix domain socket should have unix:// as url-scheme");
#endif
        return false;
    }
}
};
}
}
OPENTELEMETRY_END_NAMESPACE
