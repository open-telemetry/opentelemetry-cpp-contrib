// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "geneva_exporter_options.h"
#include <opentelemetry/exporters/fluentd/common/socket_tools.h>
#include <opentelemetry/exporters/fluentd/log/fluentd_exporter.h>
#include <opentelemetry/sdk/logs/batch_log_record_processor.h>
#include <opentelemetry/sdk/logs/logger_provider.h>
#include <opentelemetry/logs/provider.h>

OPENTELEMETRY_BEGIN_NAMESPACE

namespace exporters {

namespace geneva {

class GenevaLoggerExporter {

public:

static inline bool InitializeGenevaExporter( const GenevaExporterOptions options ) {
    // Use only unix-domain IPC for agent connectivity
    if (options.socket_endpoint.size() > kUnixDomainSchemeLength  && options.socket_endpoint.substr(0, kUnixDomainSchemeLength) == std::string(kUnixDomainScheme)){
        opentelemetry::exporter::fluentd::common::FluentdExporterOptions  fluentd_options;
        fluentd_options.retry_count = options.retry_count;
        fluentd_options.endpoint = options.socket_endpoint;
        auto exporter = std::unique_ptr<opentelemetry::sdk::logs::LogRecordExporter>(
            new opentelemetry::exporter::fluentd::logs::FluentdExporter(fluentd_options));
        auto processor = std::unique_ptr<opentelemetry::sdk::logs::LogRecordProcessor>(
            new opentelemetry::sdk::logs::BatchLogRecordProcessor(std::move(exporter), options.max_queue_size, options.schedule_delay_millis, options.max_export_batch_size));
        auto provider = std::shared_ptr<opentelemetry::logs::LoggerProvider>(
            new opentelemetry::sdk::logs::LoggerProvider(std::move(processor)));
        // Set the global logger provider
        opentelemetry::logs::Provider::SetLoggerProvider(provider);
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
