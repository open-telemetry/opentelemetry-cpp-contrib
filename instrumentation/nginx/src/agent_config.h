#pragma once

#include <string>

extern "C" {
#include <ngx_core.h>
}

enum OtelProcessorType
{
  OtelProcessorSimple,
  OtelProcessorBatch
};

struct OtelNgxAgentConfig
{
  struct
  {
    std::string endpoint;
  } exporter;

  struct
  {
    std::string name;
  } service;

  struct
  {
    OtelProcessorType type = OtelProcessorBatch;

    struct
    {
      uint32_t maxQueueSize        = 2048;
      uint32_t maxExportBatchSize  = 512;
      uint32_t scheduleDelayMillis = 5000;
    } batch;
  } processor;

  std::string sampler = "parentbased_always_on";
  double samplerRatio = 1.0;
};
