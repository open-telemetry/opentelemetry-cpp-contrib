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

// file header
#include "ApacheConfig.h"
// apache headers
#include <apr_strings.h> // apr_pstrcat
#include <iostream>
#include <memory>
#include <sstream>
// module headers
#include "ApacheHooks.h"
#include "ApacheTracing.h"

std::unordered_map<std::string, std::shared_ptr<WebserverContext> > ApacheConfigHandlers::m_webServerContexts;

//--------------------------------------------------------------------------
// COMMAND (CONFIGURATION DIRECTIVE) HANDLERS
//
// If a command handler encounters a problem processing the directive, it
// signals this fact by returning a non-NULL pointer to a string
// describing the problem.
//
// The magic return value DECLINE_CMD is used to deal with directives
// that might be declared by multiple modules.  If the command handler
// returns NULL, the directive was processed; if it returns DECLINE_CMD,
// the next module (if any) that declares the directive is given a chance
// at it.  If it returns any other value, it's treated as the text of an
// error message.
//--------------------------------------------------------------------------

const char* ApacheConfigHandlers::helperChar(
        cmd_parms* cmd,
        otel_cfg* cfg,
        const char* arg,
        const char*& var,
        int& initialized,
        const char* varName)
{
    var = apr_pstrdup(cmd->pool, arg);
    initialized = 1;
    ApacheTracing::writeTrace(cmd->server, "Config", "%s(%s)", varName, (arg ? arg : ""));
    return NULL;
}

const char* ApacheConfigHandlers::helperInt(
        cmd_parms* cmd,
        otel_cfg* cfg,
        const char* arg,
        int& var,
        int& initialized,
        const char* varName)
{
    var = !strcasecmp(arg, "on") ? 1 : 0;
    initialized = 1;
    ApacheTracing::writeTrace(cmd->server, "Config", "%s(%s)", varName, (arg ? arg : ""));
    return NULL;
}

void ApacheConfigHandlers::insertWebserverContext(cmd_parms* cmd, const otel_cfg* cfg)
{
    std::string contextName = computeContextName(cfg);
    if (contextName.empty())
    {
        ApacheTracing::writeError(
                cmd->server, "Config", "Cannot create context name (%s,%s,%s)",
                cfg->serviceNamespace, cfg->serviceName, cfg->serviceInstanceId);
        return;
    }

    ApacheTracing::writeTrace(
            cmd->server, "Config", "Context %s:%s,%s,%s",
            contextName.c_str(), cfg->serviceNamespace, cfg->serviceName, cfg->serviceInstanceId);

    auto it = m_webServerContexts.find(contextName);
    if (it != m_webServerContexts.end()) // context already exists
    {
        if (it->second->m_serviceNamespace != cfg->serviceNamespace ||
            it->second->m_serviceName != cfg->serviceName ||
            it->second->m_serviceInstanceId != cfg->serviceInstanceId)
        {
            // Context name exists but the content is different. Issue error message.
            ApacheTracing::writeError(
                    cmd->server, "Config", "Context %s already exists", contextName.c_str());
        }

        return;
    }

    m_webServerContexts[contextName] =
            std::make_shared<WebserverContext>(
                    cfg->serviceNamespace, cfg->serviceName, cfg->serviceInstanceId);
}

//  int otelEnabled;
//  int otelEnabled_initialized;
const char* ApacheConfigHandlers::otel_set_enabled(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperInt(cmd, cfg, arg, cfg->otelEnabled, cfg->otelEnabled_initialized, "otel_set_enabled");
}

//  char *otelExporterEndpoint;
//  int otelExporterEndpoint_initialized;
const char* ApacheConfigHandlers::otel_set_otelExporterEndpoint(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperChar(cmd, cfg, arg, cfg->otelExporterEndpoint, cfg->otelExporterEndpoint_initialized, "otel_set_otelExporterEndpoint");
}

//  char *otelExporterOtlpHeaders;
//  int otelExporterOtlpHeaders_initialized;
const char* ApacheConfigHandlers::otel_set_otelExporterOtlpHeaders(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperChar(cmd, cfg, arg, cfg->otelExporterOtlpHeaders, cfg->otelExporterOtlpHeaders_initialized, "otel_set_otelExporterOtlpHeaders");
}

//  char *otelSslEnabled;
//  int otelSslEnabled_initialized;
const char* ApacheConfigHandlers::otel_set_otelSslEnabled(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperInt(cmd, cfg, arg, cfg->otelSslEnabled, cfg->otelSslEnabled_initialized, "otel_set_otelSslEnabled");
}

//  char *otelSslCertificatePath;
//  int otelSslCertificatePath_initialized;
const char* ApacheConfigHandlers::otel_set_otelSslCertificatePath(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperChar(cmd, cfg, arg, cfg->otelSslCertificatePath, cfg->otelSslCertificatePath_initialized, "otel_set_otelSslCertificatePath");
}

//  char *otelExporterType;
//  int otelExporterType_initialized;
const char* ApacheConfigHandlers::otel_set_otelExporterType(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperChar(cmd, cfg, arg, cfg->otelExporterType, cfg->otelExporterType_initialized, "otel_set_otelExporterType");
}

//  char *otelProcessorType;
//  int otelProcessorType_initialized;
const char* ApacheConfigHandlers::otel_set_otelProcessorType(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperChar(cmd, cfg, arg, cfg->otelProcessorType, cfg->otelProcessorType_initialized, "otel_set_otelProcessorType");
}

//  char *otelSamplerType;
//  int otelSamplerType_initialized;
const char* ApacheConfigHandlers::otel_set_otelSamplerType(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperChar(cmd, cfg, arg, cfg->otelSamplerType, cfg->otelSamplerType_initialized, "otel_set_otelSamplerType");
}

//  char *serviceNamespace;
//  int serviceNamespace_initialized;
const char* ApacheConfigHandlers::otel_set_serviceNamespace(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    const char* rc = helperChar(cmd, cfg, arg, cfg->serviceNamespace, cfg->serviceNamespace_initialized, "otel_set_serviceNamespace");
    if (cfg->serviceNamespace_initialized && cfg->serviceName_initialized && cfg->serviceInstanceId_initialized)
    {
        // There are separate Apache directives for setting serviceNamespace, serviceName and serviceInstanceId.
        // We need to insert into the context hashtable only when all three have been initialized
        insertWebserverContext(cmd, cfg);
    }
    return rc;
}

//  char *serviceName;
//  int serviceName_initialized;
const char* ApacheConfigHandlers::otel_set_serviceName(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    const char* rc = helperChar(cmd, cfg, arg, cfg->serviceName, cfg->serviceName_initialized, "otel_set_serviceName");
    if (cfg->serviceNamespace_initialized && cfg->serviceName_initialized && cfg->serviceInstanceId_initialized)
    {
        // There are separate Apache directives for setting serviceNamespace, serviceName and serviceInstanceId.
        // We need to insert into the context hashtable only when all three have been initialized
        insertWebserverContext(cmd, cfg);
    }
    return rc;
}

//  char *serviceInstanceId;
//  int serviceInstanceId_initialized;
const char* ApacheConfigHandlers::otel_set_serviceInstanceId(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    const char* rc = helperChar(cmd, cfg, arg, cfg->serviceInstanceId, cfg->serviceInstanceId_initialized, "otel_set_serviceInstanceId");
    if (cfg->serviceNamespace_initialized && cfg->serviceName_initialized && cfg->serviceInstanceId_initialized)
    {
        // There are separate Apache directives for setting serviceNamespace, serviceName and serviceInstanceId.
        // We need to insert into the context hashtable only when all three have been initialized
        insertWebserverContext(cmd, cfg);
    }
    return rc;
}

//  char *otelMaxQueueSize;
//  int otelMaxQueueSize_initialized;
const char* ApacheConfigHandlers::otel_set_otelMaxQueueSize(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperChar(cmd, cfg, arg, cfg->otelMaxQueueSize, cfg->otelMaxQueueSize_initialized, "otel_set_otelMaxQueueSize");
}

//  char *otelScheduledDelay;
//  int otelScheduledDelay_initialized;
const char* ApacheConfigHandlers::otel_set_otelScheduledDelay(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperChar(cmd, cfg, arg, cfg->otelScheduledDelay, cfg->otelScheduledDelay_initialized, "otel_set_otelScheduledDelay");
}

//  char *otelExportTimeout;
//  int otelExportTimeout_initialized;
const char* ApacheConfigHandlers::otel_set_otelExportTimeout(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperChar(cmd, cfg, arg, cfg->otelExportTimeout, cfg->otelExportTimeout_initialized, "otel_set_otelExportTimeout");
}

//  char *otelMaxExportBatchSize;
//  int otelMaxExportBatchSize_initialized;
const char* ApacheConfigHandlers::otel_set_otelMaxExportBatchSize(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperChar(cmd, cfg, arg, cfg->otelMaxExportBatchSize, cfg->otelMaxExportBatchSize_initialized, "otel_set_otelMaxExportBatchSize");
}

//  int resolveBackends;
//  int resolveBackends_initialized;
const char* ApacheConfigHandlers::otel_set_resolveBackends(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    return helperInt(cmd, cfg, arg, cfg->resolveBackends, cfg->resolveBackends_initialized, "otel_set_resolveBackends");
}

//  int traceAsError;
//  int traceAsError_initialized;
const char* ApacheConfigHandlers::otel_set_traceAsError(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    helperInt(cmd, cfg, arg, cfg->traceAsError, cfg->traceAsError_initialized, "otel_set_traceAsError");
    ApacheTracing::m_traceAsErrorFromUser = cfg->traceAsError;
    return NULL;
}

//  int reportAllInstrumentedModules;
//  int reportAllInstrumentedModules_initialized;
const char* ApacheConfigHandlers::otel_set_reportAllInstrumentedModules(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    helperInt(
            cmd,
            cfg,
            arg,
            cfg->reportAllInstrumentedModules,
            cfg->reportAllInstrumentedModules_initialized,
            "otel_set_reportAllInstrumentedModules");
    ApacheHooks::m_reportAllStages = cfg->reportAllInstrumentedModules;
    return NULL;
}

// TODO: Commented out the variables used for redaction of PII from URL and cookies. They will be uncommented later on.

// int maskCookie
// int maskCookie_intialized
const char* ApacheConfigHandlers::otel_set_maskCookie(cmd_parms *cmd, void *conf, const char *arg)
{
   otel_cfg* cfg = (otel_cfg*) conf;
   helperInt(cmd, cfg, arg, cfg->maskCookie, cfg->maskCookie_initialized, "otel_set_maskCookie");
   // ApacheHooks::m_maskCookie = cfg->maskCookie;
   return NULL;
}

// char *cookiePattern
// int cookiePattern_initialized
const char* ApacheConfigHandlers::otel_set_cookiePattern(cmd_parms *cmd, void *conf, const char *arg)
{
   otel_cfg* cfg = (otel_cfg*) conf;
   helperChar(cmd, cfg, arg, cfg->cookiePattern, cfg->cookiePattern_initialized, "otel_set_cookiePattern");
   // ApacheHooks::m_cookiePattern = cfg->cookiePattern;
   return NULL;
}

// int maskSmUser
// int maskSmUser_intialized
const char* ApacheConfigHandlers::otel_set_maskSmUser(cmd_parms *cmd, void *conf, const char *arg)
{
   otel_cfg* cfg = (otel_cfg*) conf;
   helperInt(cmd, cfg, arg, cfg->maskSmUser, cfg->maskSmUser_initialized, "otel_set_maskSmUser");
   // ApacheHooks::m_maskSmUser = cfg->maskSmUser;
   return NULL;
}
// char *delimiter
// int delimiter_initialized
const char* ApacheConfigHandlers::otel_set_delimiter(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    helperChar(cmd, cfg, arg, cfg->delimiter, cfg->delimiter_initialized, "otel_set_delimiter");
    // UrlFilterManager::m_delimiter = cfg->delimiter;
    if (cfg->delimiter_initialized && cfg->segment_initialized && cfg->matchFilter_initialized && cfg->matchPattern_initialized)
    {
        // We initialize URL filters only when delimiter, segment, matchFilter and matchPattern have been initialized
        // UrlFilterManager::initializeUrlFilters();
    }
    return NULL;
}

// char *segment
// int segment_initialized
const char* ApacheConfigHandlers::otel_set_segment(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    helperChar(cmd, cfg, arg, cfg->segment, cfg->segment_initialized, "otel_set_segment");
    // UrlFilterManager::m_segment = cfg->segment;
    if (cfg->delimiter_initialized && cfg->segment_initialized && cfg->matchFilter_initialized && cfg->matchPattern_initialized)
    {
        // We initialize URL filters only when delimiter, segment, matchFilter and matchPattern have been initialized
        // UrlFilterManager::initializeUrlFilters();
    }
    return NULL;
}

// char *matchFilter
// int matchFilter_initialized
const char* ApacheConfigHandlers::otel_set_matchFilter(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    helperChar(cmd, cfg, arg, cfg->matchFilter, cfg->matchFilter_initialized, "otel_set_matchFilter");
    // UrlFilterManager::m_matchFilter = cfg->matchFilter;
    if (cfg->delimiter_initialized && cfg->segment_initialized && cfg->matchFilter_initialized && cfg->matchPattern_initialized)
    {
        // We initialize URL filters only when delimiter, segment, matchFilter and matchPattern have been initialized
        // UrlFilterManager::initializeUrlFilters();
    }
    return NULL;
}

// char *matchPattern
// int matchPattern_initialized
const char* ApacheConfigHandlers::otel_set_matchPattern(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    helperChar(cmd, cfg, arg, cfg->matchPattern, cfg->matchPattern_initialized, "otel_set_matchPattern");
    // UrlFilterManager::m_matchPattern = cfg->matchPattern;
    if (cfg->delimiter_initialized && cfg->segment_initialized && cfg->matchFilter_initialized && cfg->matchPattern_initialized)
    {
        // We initialize URL filters only when delimiter, segment, matchFilter and matchPattern have been initialized
        // UrlFilterManager::initializeUrlFilters();
    }
    return NULL;
}

// char *segmentType
// int segmentType_initialized
const char* ApacheConfigHandlers::otel_set_segmentType(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    helperChar(cmd, cfg, arg, cfg->segmentType, cfg->segmentType_initialized, "otel_set_segmentType");
    if (cfg->segmentType_initialized && cfg->segmentParameter_initialized)
    {

    }
    return NULL;
}

// char *segmentParameter
// int segmentParameter_initialized
const char* ApacheConfigHandlers::otel_set_segmentParameter(cmd_parms *cmd, void *conf, const char *arg)
{
    otel_cfg* cfg = (otel_cfg*) conf;
    helperChar(cmd, cfg, arg, cfg->segmentParameter, cfg->segmentParameter_initialized, "otel_set_segmentParameter");
    if (cfg->segmentType_initialized && cfg->segmentParameter_initialized)
    {

    }
    return NULL;
}

std::string ApacheConfigHandlers::computeContextName(const otel_cfg* cfg)
{
    if (!cfg->serviceNamespace_initialized || !cfg->serviceName_initialized || !cfg->serviceInstanceId_initialized)
    {
        return std::string();
    }

    std::ostringstream oss;
    oss << cfg->serviceNamespace << ":" << cfg->serviceName<< ":" << cfg->serviceInstanceId;
    return oss.str();
}

const char* ApacheConfigHandlers::otel_add_webserver_context(
        cmd_parms* cmd,
        void* conf,
        const char* serviceNamespace,
        const char* serviceName,
        const char* serviceInstanceId)
{
    otel_cfg* cfg = (otel_cfg*) conf;

    cfg->serviceNamespace = apr_pstrdup(cmd->pool, serviceNamespace);
    cfg->serviceNamespace_initialized = 1;
    cfg->serviceName = apr_pstrdup(cmd->pool, serviceName);
    cfg->serviceName_initialized = 1;
    cfg->serviceInstanceId = apr_pstrdup(cmd->pool, serviceInstanceId);
    cfg->serviceInstanceId_initialized = 1;

    insertWebserverContext(cmd, cfg);
    return NULL;
}

void otel_cfg::init()
{
    // Agent to Controller Connection Configuration
    // otelEnabled                 OPTIONAL: 0 for false, 1 for true (defaults to true)
    otelEnabled = 1;
    otelEnabled_initialized = 0;

    // otelExporterEndpoint         REQUIRED: Collecter endpoint where the OpenTelemetry Exporter inside OTel SDK sends traces
    otelExporterEndpoint = "";
    otelExporterEndpoint_initialized = 0;

    // otelExporterOtlpHeaders         Optional: OTLP headers as key value pairs
    otelExporterOtlpHeaders = "";
    otelExporterOtlpHeaders_initialized = 0;

    // otelSslEnabled       OPTIONAL: Decides whether the connection to the endpoint is secured
    otelSslEnabled = 0;
    otelSslEnabled_initialized = 0;

    // otelSslCertificatePath  OPTIONAL: SSL Certificate path
    otelSslCertificatePath = "";
    otelSslCertificatePath_initialized = 0;

    // otelExporterType             OPTIONAL: Type of exporter to be configured in TracerProvider of OTel SDK embedded into Agent
    otelExporterType = "OTLP";
    otelExporterType_initialized = 0;

    // otelProcessorType            OPTIONAL: Decision on how to pass finished and export-friendly span data representation to configured span exporter
    otelProcessorType = "BATCH";
    otelProcessorType_initialized = 0;

    // otelSamplerType              OPTIONAL: Type of Otel Sampler
    otelSamplerType = "ALWAYSON";
    otelSamplerType_initialized = 0;

    // serviceNamespace             REQUIRED: A namespace for the ServiceName;
    serviceNamespace = "";
    serviceNamespace_initialized = 0;

    // serviceName                  REQUIRED: Logical name of the service;
    serviceName = "";
    serviceName_initialized = 0;

    // serviceInstanceId            REQUIRED: The string ID of the service instance. Distinguish between instances of a service
    serviceInstanceId = "";
    serviceInstanceId_initialized = 0;

    // otelMaxQueueSize             OPTIONAL: The maximum queue size. After the size is reached spans are dropped
    otelMaxQueueSize = "2048";
    otelMaxQueueSize_initialized = 0;

    // otelScheduledDelay           OPTIONAL: The delay interval in milliseconds between two consecutive exports
    otelScheduledDelay = "5000";
    otelScheduledDelay_initialized = 0;

    // otelExportTimeout            OPTIONAL: How long the export can run in milliseconds before it is cancelled
    otelExportTimeout = "30000";
    otelExportTimeout_initialized = 0;

    // otelMaxExportBatchSize       OPTIONAL: The maximum batch size of every export. It must be smaller or equal to maxQueueSize
    otelMaxExportBatchSize = "512";
    otelMaxExportBatchSize_initialized = 0;

    // resolveBackends              OPTIONAL: resolve backends as a tier
    resolveBackends = 1;
    resolveBackends_initialized = 0;

    // traceAsError                 OPTIONAL: trace level for logging to Apache log
    traceAsError = 0;
    traceAsError_initialized = 0;

    // reportAllInstrumentedModules OPTIONAL: report ALL instrumented modules instead of just HANDLER modules
    reportAllInstrumentedModules = 0;
    reportAllInstrumentedModules_initialized = 0;

    // maskCookie                   OPTIONAL: masking cookie values
    maskCookie = 0;
    maskCookie_initialized = 0;

    // cookiePattern            OPTIONAL: Required only if there is a need to mask part of cookie
    cookiePattern = "";
    cookiePattern_initialized = 0;

    //maskSmUser                OPTIONAL: masking SM_USER values
    maskSmUser = 0;
    maskSmUser_initialized = 0;

    //delimiter                 OPTIONAL: Required only if there is a need to redact certain URL segments
    delimiter = "";
    delimiter_initialized = 0;

    //segment                   OPTIONAL: Required only if there is a need to redact certain URL segments
    segment = "";
    segment_initialized = 0;

    //matchFilter              OPTIONAL: Required only if there is a need to redact certain URL segments
    matchFilter = "";
    matchFilter_initialized = 0;

    //matchPattern             OPTIONAL: Required only if there is a need to redact certain URL segments
    matchPattern = "";
    matchPattern_initialized = 0;

    //segmentType             OPTIONAL:
    segmentType = "FIRST";
    segmentType_initialized = 0;

    //segmentParameter             OPTIONAL:
    segmentParameter = "2";
    segmentParameter_initialized = 0;
}

bool otel_cfg::validate(const request_rec *r)
{
    bool config_valid = true;

    // Agent to Controller Connection Configuration
    // otelEnabled             - should be either 1 or 0
    if ((otelEnabled != 0) && (otelEnabled != 1))
    {
        ApacheTracing::writeError(r->server, __func__, "Enabled flag is invalid - otel not enabled");
        config_valid = false;
    }

    // otelExporterEndpoint          - must be non empty
    if (strlen(otelExporterEndpoint) < 1)
    {
        ApacheTracing::writeError(r->server, __func__, "Otel Exporter Endpoint not specified - otel not enabled");
        config_valid = false;
    }

    if ((otelSslEnabled != 0) && strlen(otelSslCertificatePath) < 1)
    {
        ApacheTracing::writeError(r->server, __func__, "Otel Exporter Certificate Path not specified - otel not enabled");
        config_valid = false;
    }

    // serviceNamespace               - must be specified
    if (strlen(serviceNamespace) < 1)
    {
        ApacheTracing::writeError(r->server, __func__, "Service Namespace not specified - otel not enabled");
        config_valid = false;
    }

    // serviceName         - must be non empty
    if (strlen(serviceName) < 1)
    {
        ApacheTracing::writeError(r->server, __func__, "Service name not specified - otel not enabled");
        config_valid = false;
    }

    // serviceInstanceId               - must be non empty
    if (strlen(serviceInstanceId) < 1)
    {
        ApacheTracing::writeError(r->server, __func__, "Service Instance ID not specified - otel not enabled");
        config_valid = false;
    }

    return config_valid;
}

// This function gets called to create a per-directory configuration
// record.  This will be called for the "default" server environment, and for
// each directory for which the parser finds any of our directives applicable.
// If a directory doesn't have any of our directives involved (i.e., they
// aren't in the .htaccess file, or a <Location>, <Directory>, or related
// block), this routine will *not* be called - the configuration for the
// closest ancestor is used.
//
// The return value is a pointer to the created module-specific
// structure.
void* ApacheConfigHandlers::otel_create_dir_config(apr_pool_t* p, char* dirspec)
{
    const char* dname = (dirspec != NULL) ? dirspec : "";

    // Allocate the space for our record from the pool supplied.
    otel_cfg* cfg = (otel_cfg*) apr_pcalloc(p, sizeof(otel_cfg));
    if (cfg) // Set Default values on creation
    {
        cfg->init(); // Set Default values on creation
        cfg->loc = apr_pstrcat(p, "DIR(", dname, ")", NULL);
    }

    // Finally, add our trace to the callback list.
    ApacheTracing::writeTrace(NULL, __func__, "(p == %p, dirspec == %s)", static_cast<void*>(p), dname);
    return static_cast<void*>(cfg);
}

// This function gets called to merge two per-directory configuration
// records.  This is typically done to cope with things like .htaccess files
// or <Location> directives for directories that are beneath one for which a
// configuration record was already created.  The routine has the
// responsibility of creating a new record and merging the contents of the
// other two into it appropriately.  If the module doesn't declare a merge
// routine, the record for the closest ancestor location (that has one) is
// used exclusively.
//
// The routine MUST NOT modify any of its arguments!
//
// The return value is a pointer to the created module-specific structure
// containing the merged values.
void* ApacheConfigHandlers::otel_merge_dir_config(apr_pool_t* p, void* parent_conf, void* newloc_conf)
{
    // the "(char*)" is to remove the compiler warning, look at otel_create_dir_config for more explanation
    otel_cfg* merged_config = (otel_cfg*) otel_create_dir_config(p, (char*)"Merged Directory Configuration");
    otel_cfg* pconf = (otel_cfg*) parent_conf;
    otel_cfg* nconf = (otel_cfg*) newloc_conf;

    // Agent to Controller Connection Configuration
    // otelEnabled             OPTIONAL: 0 for false, 1 for true (defaults to false)
    merged_config->otelEnabled = nconf->otelEnabled_initialized ? nconf->otelEnabled : pconf->otelEnabled;
    merged_config->otelEnabled_initialized = 1;

    // otelExporterEndpoint          REQUIRED: Collector endpoint where the OpenTelemetry Exporter inside OTel SDK sends traces
    merged_config->otelExporterEndpoint = nconf->otelExporterEndpoint_initialized ?
            apr_pstrdup(p, nconf->otelExporterEndpoint) : apr_pstrdup(p, pconf->otelExporterEndpoint);
    merged_config->otelExporterEndpoint_initialized = 1;

    // otelSslEnabled
    merged_config->otelSslEnabled = nconf->otelSslEnabled_initialized ?
            nconf->otelSslEnabled : pconf->otelSslEnabled;
    merged_config->otelSslEnabled_initialized = 1;

    // otelSslEnabled
    merged_config->otelSslCertificatePath = nconf->otelSslCertificatePath_initialized ?
            apr_pstrdup(p, nconf->otelSslCertificatePath) : apr_pstrdup(p, pconf->otelSslCertificatePath);
    merged_config->otelSslCertificatePath_initialized = 1;

    // otelExporterType          OPTIONAL: Type of exporter to be configured in TracerProvider of OTel SDK embedded into Agent
    merged_config->otelExporterType = nconf->otelExporterType_initialized ?
            apr_pstrdup(p, nconf->otelExporterType) : apr_pstrdup(p, pconf->otelExporterType);
    merged_config->otelExporterType_initialized = 1;

    // otelProcessorType           OPTIONAL: Decision on how to pass finished and export-friendly span data representation to configured span exporter
    merged_config->otelProcessorType = nconf->otelProcessorType_initialized ?
            apr_pstrdup(p, nconf->otelProcessorType) : apr_pstrdup(p, pconf->otelProcessorType);
    merged_config->otelProcessorType_initialized = 1;

    // otelSamplerType             OPTIONAL: Type of Otel Sampler
    merged_config->otelSamplerType = nconf->otelSamplerType_initialized ?
            apr_pstrdup(p, nconf->otelSamplerType) : apr_pstrdup(p, pconf->otelSamplerType);
    merged_config->otelSamplerType_initialized = 1;

    // serviceNamespace          REQUIRED: A namespace for the ServiceName;
    merged_config->serviceNamespace = nconf->serviceNamespace_initialized ?
            apr_pstrdup(p, nconf->serviceNamespace) : apr_pstrdup(p, pconf->serviceNamespace);
    merged_config->serviceNamespace_initialized = 1;

    // serviceName               REQUIRED: Logical name of the service;
    merged_config->serviceName = nconf->serviceName_initialized ?
            apr_pstrdup(p, nconf->serviceName) : apr_pstrdup(p, pconf->serviceName);
    merged_config->serviceName_initialized = 1;

    // serviceInstanceId         REQUIRED: The string ID of the service instance. Distinguish between instances of a service
    merged_config->serviceInstanceId = nconf->serviceInstanceId_initialized ?
            apr_pstrdup(p, nconf->serviceInstanceId) : apr_pstrdup(p, pconf->serviceInstanceId);
    merged_config->serviceInstanceId_initialized = 1;

    // otelMaxQueueSize              OPTIONAL: The maximum queue size. After the size is reached spans are dropped
    merged_config->otelMaxQueueSize = nconf->otelMaxQueueSize_initialized ?
            apr_pstrdup(p, nconf->otelMaxQueueSize) : apr_pstrdup(p, pconf->otelMaxQueueSize);
    merged_config->otelMaxQueueSize_initialized = 1;

    // otelScheduledDelay         OPTIONAL: The delay interval in milliseconds between two consecutive exports
    merged_config->otelScheduledDelay = nconf->otelScheduledDelay_initialized ?
            apr_pstrdup(p, nconf->otelScheduledDelay) : apr_pstrdup(p, pconf->otelScheduledDelay);
    merged_config->otelScheduledDelay_initialized = 1;

    // otelExportTimeout                OPTIONAL: How long the export can run in milliseconds before it is cancelled
    merged_config->otelExportTimeout = nconf->otelExportTimeout_initialized ?
            apr_pstrdup(p, nconf->otelExportTimeout) : apr_pstrdup(p, pconf->otelExportTimeout);
    merged_config->otelExportTimeout_initialized = 1;

    // otelMaxExportBatchSize                OPTIONAL: The maximum batch size of every export. It must be smaller or equal to maxQueueSize
    merged_config->otelMaxExportBatchSize = nconf->otelMaxExportBatchSize_initialized ?
            apr_pstrdup(p, nconf->otelMaxExportBatchSize) : apr_pstrdup(p, pconf->otelMaxExportBatchSize);
    merged_config->otelMaxExportBatchSize_initialized = 1;

    merged_config->resolveBackends = nconf->resolveBackends_initialized ?
            nconf->resolveBackends : pconf->resolveBackends;
    merged_config->resolveBackends_initialized = 1;
    merged_config->traceAsError = nconf->traceAsError_initialized ? nconf->traceAsError : pconf->traceAsError;
    merged_config->traceAsError_initialized = 1;
    merged_config->reportAllInstrumentedModules = nconf->reportAllInstrumentedModules_initialized ?
            nconf->reportAllInstrumentedModules : pconf->reportAllInstrumentedModules;
    merged_config->reportAllInstrumentedModules_initialized = 1;

    // for masking cookies
    merged_config->maskCookie = nconf->maskCookie_initialized ? nconf->maskCookie : pconf->maskCookie;
    merged_config->maskCookie_initialized = 1;
    merged_config->cookiePattern = nconf->cookiePattern_initialized ? nconf->cookiePattern : pconf->cookiePattern;
    merged_config->cookiePattern_initialized = 1;

    // for masking SM_USER
    merged_config->maskSmUser = nconf->maskSmUser_initialized ? nconf->maskSmUser : pconf->maskSmUser;
    merged_config->maskSmUser_initialized = 1;

    // for redacting URL segments
    merged_config->delimiter = nconf->delimiter_initialized ? nconf->delimiter : pconf->delimiter;
    merged_config->delimiter_initialized = 1;

    merged_config->segment = nconf->segment_initialized ? nconf->segment : pconf->segment;
    merged_config->segment_initialized = 1;

    merged_config->matchFilter  = nconf->matchFilter_initialized ? nconf->matchFilter  : pconf->matchFilter ;
    merged_config->matchFilter_initialized = 1;

    merged_config->matchPattern = nconf->matchPattern_initialized ? nconf->matchPattern : pconf->matchPattern;
    merged_config->matchPattern_initialized = 1;

    merged_config->segmentType = nconf->segmentType_initialized ? nconf->segmentType : pconf->segmentType;
    merged_config->segmentType_initialized = 1;

    merged_config->segmentParameter = nconf->segmentParameter_initialized ? nconf->segmentParameter : pconf->segmentParameter;
    merged_config->segmentParameter_initialized = 1;


    ApacheTracing::writeTrace(NULL, __func__,
            "(p == %p, parent_conf == %p, newloc_conf == %p)",
            static_cast<void*>(p),
            static_cast<void*>(parent_conf),
            static_cast<void*>(newloc_conf));

    return static_cast<void*>(merged_config);
}

otel_cfg* ApacheConfigHandlers::getConfig(const request_rec* r)
{
    otel_cfg* cfg = ApacheConfigHandlers::our_dconfig(r);

    if (cfg && cfg->validate(r))
    {
        return cfg;
    }

    return NULL;
}

/*
    Function to set user data/config in the process pool
*/
otel_cfg* ApacheConfigHandlers::getProcessConfig(const request_rec* r)
{
    otel_cfg* our_config = ApacheConfigHandlers::getConfig(r);

    otel_cfg *process_cfg;
    process_cfg = (otel_cfg *) apr_pcalloc(r->server->process->pool, sizeof(otel_cfg));

    process_cfg->otelEnabled = our_config->otelEnabled;
    process_cfg->otelEnabled_initialized = our_config->otelEnabled_initialized;

    process_cfg->otelExporterEndpoint = apr_pstrdup(r->server->process->pool, our_config->otelExporterEndpoint);
    process_cfg->otelExporterEndpoint_initialized = our_config->otelExporterEndpoint_initialized;

    process_cfg->otelExporterOtlpHeaders = apr_pstrdup(r->server->process->pool, our_config->otelExporterOtlpHeaders);
    process_cfg->otelExporterOtlpHeaders_initialized = our_config->otelExporterOtlpHeaders_initialized;

    process_cfg->otelSslEnabled = our_config->otelSslEnabled;
    process_cfg->otelSslEnabled_initialized = our_config->otelSslEnabled_initialized;

    process_cfg->otelSslCertificatePath = apr_pstrdup(r->server->process->pool, our_config->otelSslCertificatePath);
    process_cfg->otelSslCertificatePath_initialized = our_config->otelSslCertificatePath_initialized;

    process_cfg->otelExporterType = apr_pstrdup(r->server->process->pool, our_config->otelExporterType);
    process_cfg->otelExporterType_initialized = our_config->otelExporterType_initialized;

    process_cfg->otelProcessorType = apr_pstrdup(r->server->process->pool, our_config->otelProcessorType);
    process_cfg->otelProcessorType_initialized = our_config->otelProcessorType_initialized;

    process_cfg->otelSamplerType = apr_pstrdup(r->server->process->pool, our_config->otelSamplerType);
    process_cfg->otelSamplerType_initialized = our_config->otelSamplerType_initialized;

    process_cfg->serviceNamespace = apr_pstrdup(r->server->process->pool, our_config->serviceNamespace);
    process_cfg->serviceNamespace_initialized = our_config->serviceNamespace_initialized;

    process_cfg->serviceName = apr_pstrdup(r->server->process->pool, our_config->serviceName);
    process_cfg->serviceName_initialized = our_config->serviceName_initialized;

    process_cfg->serviceInstanceId = apr_pstrdup(r->server->process->pool, our_config->serviceInstanceId);
    process_cfg->serviceInstanceId_initialized = our_config->serviceInstanceId_initialized;

    process_cfg->otelMaxQueueSize = apr_pstrdup(r->server->process->pool, our_config->otelMaxQueueSize);
    process_cfg->otelMaxQueueSize_initialized = our_config->otelMaxQueueSize_initialized;

    process_cfg->otelScheduledDelay = apr_pstrdup(r->server->process->pool, our_config->otelScheduledDelay);
    process_cfg->otelScheduledDelay_initialized = our_config->otelScheduledDelay_initialized;

    process_cfg->otelExportTimeout = apr_pstrdup(r->server->process->pool, our_config->otelExportTimeout);
    process_cfg->otelExportTimeout_initialized = our_config->otelExportTimeout_initialized;

    process_cfg->otelMaxExportBatchSize = apr_pstrdup(r->server->process->pool, our_config->otelMaxExportBatchSize);
    process_cfg->otelMaxExportBatchSize_initialized = our_config->otelMaxExportBatchSize_initialized;

    process_cfg->resolveBackends = our_config->resolveBackends;
    process_cfg->resolveBackends_initialized = our_config->resolveBackends_initialized;

    process_cfg->traceAsError = our_config->traceAsError;
    process_cfg->traceAsError_initialized = our_config->traceAsError_initialized;

    process_cfg->reportAllInstrumentedModules = our_config->reportAllInstrumentedModules;
    process_cfg->reportAllInstrumentedModules_initialized = our_config->reportAllInstrumentedModules_initialized;

    return process_cfg;
}

/*
    Function to mask the PII data, thereby preventing it from getting logged
    directly into logs. **Not used as of now**.

std::string ApacheConfigHandlers::hashPassword(const char* arg)
{
    if(!arg || strlen(arg)==0)
    {
        return std::string();
    }

    return std::string(SIZE,'*');
}
*/

/*
    Function to log config.
*/
void ApacheConfigHandlers::traceConfig(const request_rec* r, const otel_cfg* cfg)
{
    ApacheTracing::writeTrace(r->server, __func__,
            "config{"
                "(Enabled=\"%d\")"
                "(OtelExporterEndpoint=\"%s\")"
                "(OtelSslEnabled=\"%d\")"
                "(OtelSslCertificatePath=\"%s\")"
                "(OtelSpanExporter=\"%s\")"
                "(OtelSpanProcessor=\"%s\")"
                "(OtelSampler=\"%s\")"
                "(ServiceNamespace=\"%s\")"
                "(ServiceName=\"%s\")"
                "(ServiceInstanceId=\"%s\")"
                "(OtelMaxQueueSize=\"%s\")"
                "(OtelScheduledDelayMillis=\"%s\")"
                "(OtelExportTimeoutMillis=\"%s\")"
                "(OtelMaxExportBatchSize=\"%s\")"
                "(ResolveBackends=\"%d\")"
                "(TraceAsError=\"%d\")"
                "(ReportAllInstrumentedModules=\"%d\")"
                "(MaskCookie=\"%d\")"
                "(MaskSmUser=\"%d\")"
                "(SegmentType=\"%s\")"
                "(SegmentParameter=\"%s\")"
            "}",
            cfg->otelEnabled,
            cfg->otelExporterEndpoint,
            cfg->otelSslEnabled,
            cfg->otelSslCertificatePath,
            cfg->otelExporterType,
            cfg->otelProcessorType,
            cfg->otelSamplerType,
            cfg->serviceNamespace,
            cfg->serviceName,
            cfg->serviceInstanceId,
            cfg->otelMaxQueueSize,
            cfg->otelScheduledDelay,
            cfg->otelExportTimeout,
            cfg->otelMaxExportBatchSize,
            cfg->resolveBackends,
            cfg->traceAsError,
            cfg->reportAllInstrumentedModules,
            cfg->maskCookie,
            cfg->maskSmUser,
            cfg->segmentType,
            cfg->segmentParameter);
}
