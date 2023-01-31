/*
* Copyright 2022, OpenTelemetry Authors. 
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef APACHECONFIG_H
#define APACHECONFIG_H
#define SIZE 5 //Fixed SIZE (intermediate value chosen as 5) for masking private data if any before logging into Apache logs

#include <memory>
#include <string>
#include <unordered_map>
#include "httpd.h" // request_rec
#include "http_config.h" // cmd_parms

class otel_cfg
{
public:
    void init();
    bool validate(const request_rec *r);

    int getOtelEnabled() { return otelEnabled; }
    int getOtelEnabledInitialized() { return otelEnabled_initialized; }

    const char* getOtelExporterType() { return otelExporterType; }
    int getOtelExporterTypeInitialized() { return otelExporterType_initialized; }

    const char* getOtelExporterEndpoint() { return otelExporterEndpoint; }
    int getOtelExporterEndpointInitialized() { return otelExporterEndpoint_initialized; }

    const char* getOtelExporterOtlpHeaders() { return otelExporterOtlpHeaders; }
    int otelExporterOtlpHeadersInitialized() { return otelExporterOtlpHeaders_initialized; }

    int getOtelSslEnabled() { return otelSslEnabled; }
    int getOtelSslEnabledInitialized() { return otelSslEnabled_initialized; }

    const char* getOtelSslCertificatePath() { return otelSslCertificatePath; }
    int getOtelSslCertificatePathInitialized() { return otelSslCertificatePath_initialized; }

    const char* getOtelProcessorType() { return otelProcessorType; }
    int getOtelProcessorTypeInitialized() { return otelProcessorType_initialized; }

    const char* getOtelSamplerType() { return otelSamplerType; }
    int getOtelSamplerTypeInitialized() { return otelSamplerType_initialized; }

    const char* getServiceNamespace() { return serviceNamespace; }
    int getServiceNamespaceInitialized() { return serviceNamespace_initialized; }

    const char* getServiceName() { return serviceName; }
    int getServiceNameInitialized() { return serviceName_initialized; }

    const char* getServiceInstanceId() { return serviceInstanceId; }
    int getServiceInstanceIdInitialized() { return serviceInstanceId_initialized; }

    const char* getOtelMaxQueueSize() { return otelMaxQueueSize; }
    int getOtelMaxQueueSizeInitialized() { return otelMaxQueueSize_initialized; }

    const char* getOtelScheduledDelay() { return otelScheduledDelay; }
    int getOtelScheduledDelayInitialized() { return otelScheduledDelay_initialized; }

    const char* getOtelExportTimeout() { return otelExportTimeout; }
    int getOtelExportTimeoutInitialized() { return otelExportTimeout_initialized; }

    const char* getOtelMaxExportBatchSize() { return otelMaxExportBatchSize; }
    int getOtelMaxExportBatchSizeInitialized() { return otelMaxExportBatchSize_initialized; }

    int getResolveBackends() { return resolveBackends; }
    int getResolveBackendsInitialized() { return resolveBackends_initialized; }

    int getTraceAsError() { return traceAsError; }
    int getTraceAsErrorInitialized() { return traceAsError_initialized; }

    int getReportAllInstrumentedModules() { return reportAllInstrumentedModules; }
    int getReportAllInstrumentedModulesInitialized() { return reportAllInstrumentedModules_initialized; }

    char* getLoc() { return loc; }

    int getMaskCookie() { return maskCookie; }
    int getMaskCookieInitialized() { return maskCookie_initialized; }

    const char* getCookiePattern() { return cookiePattern; }
    int getCookiePatternInitialized() { return cookiePattern_initialized; }

    int getMaskSmUser() { return maskSmUser; }
    int getMaskSmUserInitialized() { return maskSmUser_initialized; }

    const char* getDelimiter() { return delimiter; }
    int getDelimiterInitialized() { return delimiter_initialized; }

    const char* getSegment() { return segment; }
    int getSegmentInitialized() { return segment_initialized; }

    const char* getMatchFilter() { return matchFilter; }
    int getMatchFilterInitialized() { return matchFilter_initialized; }

    const char* getMatchPattern() { return matchPattern; }
    int getMatchPatternInitialized() { return matchPattern_initialized; }

    const char* getSegmentType() { return segmentType; }
    int getSegmentTypeInitialized() { return segmentType_initialized; }

    const char* getSegmentParameter() { return segmentParameter; }
    int getSegmentParameterInitialized() { return segmentParameter_initialized; }

    friend class ApacheConfigHandlers;

private:
    // Agent to Controller Connection Configuration
    int otelEnabled;                    // OPTIONAL: 0 for false, 1 for true (defaults to true)
    int otelEnabled_initialized;

    const char *otelExporterType;       // OPTIONAL: Type of exporter to be configured in TracerProvider of OTel SDK embedded into Agent
    int otelExporterType_initialized;

    const char *otelExporterEndpoint;   // REQUIRED: Collector endpoint where the OpenTelemetry Exporter inside OTel SDK sends traces
    int otelExporterEndpoint_initialized;

    const char *otelExporterOtlpHeaders;   // OPTIONAL: AppDynamics  Custom metadata for OTEL Exporter EX: OTEL_EXPORTER_OTLP_HEADERS="api-key=key,other-config-value=value"
    int otelExporterOtlpHeaders_initialized;

    int otelSslEnabled;      // OPTIONAL: Decision whether connection to the Exporter endpoint is secured
    int otelSslEnabled_initialized;

    const char *otelSslCertificatePath;  // OPTIONAL: Certificate path to be mentioned if OtelSslEnabled is set;
    int otelSslCertificatePath_initialized;

    const char *otelProcessorType;      // OPTIONAL: Decision on how to pass finished and export-friendly span data representation to configured span exporter
    int otelProcessorType_initialized;

    const char *otelSamplerType;        // OPTIONAL: Type of Otel Sampler
    int otelSamplerType_initialized;

    const char *serviceNamespace;       // REQUIRED: A namespace for the ServiceName;
    int serviceNamespace_initialized;

    const char *serviceName;            // REQUIRED: Logical name of the service;
    int serviceName_initialized;

    const char *serviceInstanceId;      // REQUIRED: The string ID of the service instance. Distinguish between instances of a service
    int serviceInstanceId_initialized;

    const char *otelMaxQueueSize;               // OPTIONAL: The maximum queue size. After the size is reached spans are dropped
    int otelMaxQueueSize_initialized;

    const char *otelScheduledDelay;             // OPTIONAL: The delay interval in milliseconds between two consecutive exports
    int otelScheduledDelay_initialized;

    const char *otelExportTimeout;              // OPTIONAL: How long the export can run in milliseconds before it is cancelled
    int otelExportTimeout_initialized;

    const char *otelMaxExportBatchSize;         // OPTIONAL: The maximum batch size of every export. It must be smaller or equal to maxQueueSize
    int otelMaxExportBatchSize_initialized;

    int resolveBackends;                // OPTIONAL: Resolve backends as a tier
    int resolveBackends_initialized;

    int traceAsError;                   // OPTIONAL: Determine whether we put all diagnostic output to error_log
    int traceAsError_initialized;

    int reportAllInstrumentedModules;   // OPTIONAL: Report ALL modules as backends instead of only HANDLER modules
    int reportAllInstrumentedModules_initialized;

    // variables for tracing calls
    char *loc;                  // Location to which this record applies.

    // GDPR: cookie masking
    int maskCookie;  // OPTIONAL: Mask Coookies
    int maskCookie_initialized;

    const char *cookiePattern; // OPTIONAL: Required only if there is a need to mask part of cookie, otherwise it will mask whole cookie if maskCookie is enabled
    int cookiePattern_initialized;

    int maskSmUser; // OPTIONAL: Mask SM_USER
    int maskSmUser_initialized;

    // URL redaction
    const char *delimiter;  // OPTIONAL: Required only if there is a need to redact certain URL segments
    int delimiter_initialized;

    const char *segment;    // OPTIONAL: Required only if there is a need to redact certain URL segments
    int segment_initialized;

    const char *matchFilter;   // OPTIONAL: Required only if there is a need to redact certain URL segments
    int matchFilter_initialized;

    const char *matchPattern;  // OPTIONAL: Required only if there is a need to redact certain URL segments
    int matchPattern_initialized;

    // Rules on how the span/BT name would be created.
    const char *segmentType;        // OPTIONAL: Possible Values are FIRST, LAST, CUSTOM..
    int segmentType_initialized;

    const char *segmentParameter;       // OPTIONAL: Should be specified if segmentType is provided.
    int segmentParameter_initialized;   // if FIRST/LAST is choosen above, segment count should be provided.
                                        // if CUSTOM is choosen, segment numbers should be provided such as 2,3

};

class WebserverContext
{
public:
    std::string m_serviceNamespace;
    std::string m_serviceName;
    std::string m_serviceInstanceId;
    // TODO: in future we may need to put in controller host/port, account name, access key, etc

    WebserverContext(const char* serviceNamespace, const char* serviceName, const char* serviceInstanceId)
            : m_serviceNamespace(serviceNamespace), m_serviceName(serviceName), m_serviceInstanceId(serviceInstanceId) {};

private:
    WebserverContext() {};
};

class ApacheConfigHandlers
{
public:
    static std::unordered_map<std::string, std::shared_ptr<WebserverContext> > m_webServerContexts;

    static const char* otel_set_enabled(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_otelExporterType(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_otelExporterEndpoint(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_otelExporterOtlpHeaders(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_otelSslEnabled(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_otelSslCertificatePath(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_otelProcessorType(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_otelSamplerType(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_serviceNamespace(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_serviceName(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_serviceInstanceId(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_otelMaxQueueSize(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_otelScheduledDelay(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_otelExportTimeout(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_otelMaxExportBatchSize(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_resolveBackends(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_traceAsError(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_reportAllInstrumentedModules(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_maskCookie(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_cookiePattern(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_maskSmUser(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_delimiter(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_segment(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_matchFilter(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_matchPattern(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_segmentType(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_set_segmentParameter(cmd_parms *cmd, void *conf, const char *arg);
    static const char* otel_add_webserver_context(
            cmd_parms* cmd,
            void* conf,
            const char* serviceNamespace,
            const char* serviceName,
            const char* serviceInstanceId);

    static void* otel_create_dir_config(apr_pool_t *p, char *dirspec);
    static void* otel_merge_dir_config(apr_pool_t *p, void *parent_conf, void *newloc_conf);

    static otel_cfg* getConfig(const request_rec* r);
    static otel_cfg* getProcessConfig(const request_rec* r);

    static std::string computeContextName(const otel_cfg* cfg);

    static std::string hashPassword(const char* arg);

    static void traceConfig(const request_rec* r, const otel_cfg* cfg);

private:
    static otel_cfg* our_dconfig(const request_rec *r);

    static const char* helperChar(
            cmd_parms* cmd,
            otel_cfg* cfg,
            const char* arg,
            const char*& var,
            int& inherit,
            const char* varName);
    static const char* helperInt(
            cmd_parms* cmd,
            otel_cfg* cfg,
            const char* arg,
            int& var,
            int& inherit,
            const char* varName);
    static void insertWebserverContext(cmd_parms* cmd, const otel_cfg* cfg);
};

#endif
