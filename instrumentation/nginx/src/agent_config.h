#pragma once

#include <string>

extern "C" {
#include <ngx_core.h>
}

enum OtelExporterType { OtelExporterOTLP, OtelExporterJaeger };
enum OtelProcessorType { OtelProcessorSimple, OtelProcessorBatch };

struct OtelNgxAgentConfig {
  struct {
    OtelExporterType type = OtelExporterOTLP;
    std::string endpoint;
  } exporter;

  struct {
    std::string name = "unknown:nginx";
  } service;

  struct {
    OtelProcessorType type = OtelProcessorSimple;

    struct {
      uint32_t maxQueueSize = 2048;
      uint32_t maxExportBatchSize = 512;
      uint32_t scheduleDelayMillis = 5000;
    } batch;
  } processor;
};

bool OtelAgentConfigLoad(const std::string& path, ngx_log_t* log, OtelNgxAgentConfig* config);
