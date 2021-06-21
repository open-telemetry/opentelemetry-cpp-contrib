#pragma once

#include <string>

extern "C" {
#include <ngx_core.h>
}

enum OtelExporterType { OtelExporterOTLP, OtelExporterJaeger };
enum OtelProcessorType { OtelProcessorSimple, OtelProcessorBatch };
enum OtelSamplerType { OtelSamplerAlwaysOn, OtelSamplerAlwaysOff, OtelSamplerTraceIdRatioBased };

struct OtelNgxAgentConfig {
  struct {
    OtelExporterType type = OtelExporterOTLP;
    std::string endpoint;
    bool use_ssl_credentials = false;
    std::string ssl_credentials_cacert_path = "";
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

  struct {
    OtelSamplerType type = OtelSamplerAlwaysOn;
    bool parentBased = false;
    double ratio = 0;
  } sampler;
};

bool OtelAgentConfigLoad(const std::string& path, ngx_log_t* log, OtelNgxAgentConfig* config);
