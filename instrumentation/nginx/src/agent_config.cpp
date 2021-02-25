#include "agent_config.h"
#include "toml.h"
#include <algorithm>
#include <stdlib.h>

struct ScopedTable {
  ScopedTable(toml_table_t* table) : table(table) {}
  ~ScopedTable() { toml_free(table); }

  toml_table_t* table;
};

static std::string FromStringDatum(toml_datum_t datum) {
  std::string val{datum.u.s};
  free(datum.u.s);
  return val;
}

static bool SetupOtlpExporter(toml_table_t* table, ngx_log_t* log, OtelNgxAgentConfig* config) {
  toml_datum_t hostVal = toml_string_in(table, "host");
  toml_datum_t portVal = toml_int_in(table, "port");

  if (!hostVal.ok) {
    ngx_log_error(NGX_LOG_ERR, log, 0, "Missing required host field for OTLP exporter");
    return false;
  }

  std::string host = FromStringDatum(hostVal);

  if (!portVal.ok) {
    ngx_log_error(NGX_LOG_ERR, log, 0, "Missing required port field for OTLP exporter");
    return false;
  }

  config->exporter.host = host;
  config->exporter.port = portVal.u.i;

  return true;
}

static bool SetupExporter(toml_table_t* root, ngx_log_t* log, OtelNgxAgentConfig* config) {
  toml_datum_t exporterVal = toml_string_in(root, "exporter");

  if (!exporterVal.ok) {
    ngx_log_error(NGX_LOG_ERR, log, 0, "Missing required exporter field");
    return false;
  }

  std::string exporter = FromStringDatum(exporterVal);

  toml_table_t* exporters = toml_table_in(root, "exporters");

  if (!exporters) {
    ngx_log_error(NGX_LOG_ERR, log, 0, "Unable to find exporters table");
    return false;
  }

  if (exporter == "otlp") {
    toml_table_t* otlp = toml_table_in(exporters, "otlp");

    if (!otlp) {
      ngx_log_error(NGX_LOG_ERR, log, 0, "Unable to find exporters.otlp");
      return false;
    }

    if (!SetupOtlpExporter(otlp, log, config)) {
      return false;
    }

    config->exporter.type = OtelExporterOTLP;
  } else {
    ngx_log_error(NGX_LOG_ERR, log, 0, "Unsupported exporter %s", exporter.c_str());
    return false;
  }

  return true;
}

static bool SetupService(toml_table_t* root, ngx_log_t*, OtelNgxAgentConfig* config) {
  toml_table_t* service = toml_table_in(root, "service");

  if (service) {
    toml_datum_t serviceName = toml_string_in(service, "name");

    if (serviceName.ok) {
      config->service.name = FromStringDatum(serviceName);
    }
  }

  return true;
}

static bool SetupProcessor(toml_table_t* root, ngx_log_t* log, OtelNgxAgentConfig* config) {
  toml_datum_t processorVal = toml_string_in(root, "processor");

  if (!processorVal.ok) {
    ngx_log_error(NGX_LOG_ERR, log, 0, "Unable to find required processor field");
    return false;
  }

  std::string processor = FromStringDatum(processorVal);

  if (processor != "batch") {
    config->processor.type = OtelProcessorSimple;
    return true;
  }

  config->processor.type = OtelProcessorBatch;

  toml_table_t* processors = toml_table_in(root, "processors");

  if (!processors) {
    // Go with the default batch processor config
    return true;
  }

  toml_table_t* batchProcessor = toml_table_in(processors, "batch");

  if (!batchProcessor) {
    return true;
  }

  toml_datum_t maxQueueSize = toml_int_in(batchProcessor, "max_queue_size");

  if (maxQueueSize.ok) {
    config->processor.batch.maxQueueSize = std::max(int64_t(1), maxQueueSize.u.i);
  }

  toml_datum_t scheduleDelayMillis = toml_int_in(batchProcessor, "schedule_delay_millis");

  if (scheduleDelayMillis.ok) {
    config->processor.batch.scheduleDelayMillis = std::max(int64_t(0), scheduleDelayMillis.u.i);
  }

  toml_datum_t maxExportBatchSize = toml_int_in(batchProcessor, "max_export_batch_size");

  if (maxExportBatchSize.ok) {
    config->processor.batch.maxExportBatchSize = std::max(int64_t(1), maxExportBatchSize.u.i);
  }

  return true;
}

bool OtelAgentConfigLoad(const std::string& path, ngx_log_t* log, OtelNgxAgentConfig* config) {
  FILE* confFile = fopen(path.c_str(), "r");

  if (!confFile) {
    ngx_log_error(NGX_LOG_ERR, log, 0, "Unable to open agent config file at %s", path.c_str());
    return false;
  }

  char errBuf[256] = {0};
  ScopedTable scopedConf{toml_parse_file(confFile, errBuf, sizeof(errBuf))};
  fclose(confFile);

  if (!scopedConf.table) {
    ngx_log_error(NGX_LOG_ERR, log, 0, "Configuration error: %s", errBuf);
    return false;
  }

  toml_table_t* root = scopedConf.table;

  if (!SetupExporter(root, log, config)) {
    return false;
  }

  if (!SetupService(root, log, config)) {
    return false;
  }

  if (!SetupProcessor(root, log, config)) {
    return false;
  }

  return true;
}
