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

/*
 * OpenTelemetry Agent Module.  Module that hooks in to apache at the relevant
 * request processing phases to allow apache to appear as an entity in the flow map.
 *
 * TODO: Add the description of the BT Logic
 */

#include "ApacheConfig.h"
#include "ApacheHooks.h"

#if __GNUC__ >= 4
    #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
    #define DLL_PUBLIC
    #define DLL_LOCAL
#endif

//--------------------------------------------------------------------------
// All of the routines have been declared now.  Here's the list of
// directives specific to our module, and information about where they
// may appear and how the command parser should pass them to us for
// processing.  Note that care must be taken to ensure that there are NO
// collisions of directive names between modules.
//--------------------------------------------------------------------------
// The function prototype below is used to get around the weak function typing used in C and
// exploited by Apache. This trick was taken from
// http://wolfgang.groogroo.com/apache-cplusplus/cplusplus/strong_types.html
//--------------------------------------------------------------------------
typedef const char *(*CMD_HAND_TYPE) ();
static const command_rec otel_cmds[] =
{
    AP_INIT_TAKE1(
            "apacheModuleEnabled",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_enabled,
            NULL,
            OR_ALL,
            "Enable or disable webserver apache module"),
    AP_INIT_TAKE1(
            "apacheModuleOtelSpanExporter",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_otelExporterType,
            NULL,
            OR_ALL,
            "Type of exporter to be configured in TracerProvider of OTel SDK embedded into Agent"),
    AP_INIT_TAKE1(
            "apacheModuleOtelExporterEndpoint",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_otelExporterEndpoint,
            NULL,
            OR_ALL,
            "Collector Endpoint where the OpenTelemetry Exporter inside OTel SDK sends traces"),
    AP_INIT_TAKE1(
            "apacheModuleOtelExporterHeaders",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_otelExporterOtlpHeaders,
            NULL,
            OR_ALL,
            "AppDynamics Otel export Headers key value pairs"),
    AP_INIT_TAKE1(
            "apacheModuleOtelSslEnabled",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_otelSslEnabled,
            NULL,
            OR_ALL,
            "Decision whether communication to backend is secured"),
    AP_INIT_TAKE1(
            "apacheModuleOtelSslCertificatePath",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_otelSslCertificatePath,
            NULL,
            OR_ALL,
            "Path to SSL certificate"),
    AP_INIT_TAKE1(
            "apacheModuleOtelSpanProcessor",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_otelProcessorType,
            NULL,
            OR_ALL,
            "Decision on how to pass finished and export-friendly span data representation to configured span exporter"),
    AP_INIT_TAKE1(
            "apacheModuleOtelSampler",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_otelSamplerType,
            NULL,
            OR_ALL,
            "Type of Otel Sampler"),
    AP_INIT_TAKE1(
            "apacheModuleServiceName",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_serviceName,
            NULL,
            OR_ALL,
            "Logical name of the service"),
    AP_INIT_TAKE1(
            "apacheModuleServiceNamespace",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_serviceNamespace,
            NULL,
            OR_ALL,
            "Logical namespace of the service;"),
    AP_INIT_TAKE1(
            "apacheModuleServiceInstanceId",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_serviceInstanceId,
            NULL,
            OR_ALL,
            "The string ID of the service instance. Distinguish between instances of a service"),
    AP_INIT_TAKE1(
            "apacheModuleOtelMaxQueueSize",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_otelMaxQueueSize,
            NULL,
            OR_ALL,
            "The maximum queue size. After the size is reached spans are dropped"),
    AP_INIT_TAKE1(
            "apacheModuleOtelScheduledDelayMillis",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_otelScheduledDelay,
            NULL,
            OR_ALL,
            "The delay interval in milliseconds between two consecutive exports"),
    AP_INIT_TAKE1(
            "apacheModuleOtelExportTimeoutMillis",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_otelExportTimeout,
            NULL,
            OR_ALL,
            "How long the export can run in milliseconds before it is cancelled"),
    AP_INIT_TAKE1(
            "apacheModuleOtelMaxExportBatchSize",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_otelMaxExportBatchSize,
            NULL,
            OR_ALL,
            "The maximum batch size of every export. It must be smaller or equal to maxQueueSize"),
    AP_INIT_TAKE1(
            "apacheModuleResolveBackends",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_resolveBackends,
            NULL,
            OR_ALL,
            "Determine if backends need to be resolved as a tier"),
    AP_INIT_TAKE1(
            "apacheModuleTraceAsError",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_traceAsError,
            NULL,
            OR_ALL,
            "Trace level for information in the Apache log"),
    AP_INIT_TAKE1(
            "apacheModuleReportAllInstrumentedModules",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_reportAllInstrumentedModules,
            NULL,
            OR_ALL,
            "Report all modules as endpoints instead of only those in the HANDLER stage"),
   AP_INIT_TAKE3(
            "apacheModuleWebserverContext",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_add_webserver_context,
            NULL,
            OR_ALL,
            "Specify webserver context for a mutli-tenant mode"),
   AP_INIT_TAKE1(
            "apacheModuleMaskCookie",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_maskCookie,
            NULL,
            OR_ALL,
            "Specify whether to mask the cookies"),
   AP_INIT_TAKE1(
            "apacheModuleCookieMatchPattern",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_cookiePattern,
            NULL,
            OR_ALL,
            "Specify the pattern of cookie name to be masked"),
   AP_INIT_TAKE1(
            "apacheModuleMaskSmUser",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_maskSmUser,
            NULL,
            OR_ALL,
            "Specify whether to mask SM_USER"),
    AP_INIT_TAKE1(
            "apacheModuleDelimiter",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_delimiter,
            NULL,
            OR_ALL,
            "Specify the character that you want to use as URL segment endpoints"),
    AP_INIT_TAKE1(
            "apacheModuleSegment",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_segment,
            NULL,
            OR_ALL,
            "Specify a comma-separated list to indicate the segments that you want the agent to filter"),
    AP_INIT_TAKE1(
            "apacheModuleMatchfilter",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_matchFilter,
            NULL,
            OR_ALL,
            "The type of filtering to be used to match the url"),
    AP_INIT_TAKE1(
            "apacheModuleMatchpattern",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_matchPattern,
            NULL,
            OR_ALL,
            "Specify the string that you want to be filtered by the match-filter"),
    AP_INIT_TAKE1(
            "apacheModuleSegmentType",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_segmentType,
            NULL,
            OR_ALL,
            "Specify the string (FIRST|LAST|CUSTOM) that you want to be filtered for Span Creation"),
    AP_INIT_TAKE1(
            "apacheModuleSegmentParameter",
            (CMD_HAND_TYPE)ApacheConfigHandlers::otel_set_segmentParameter,
            NULL,
            OR_ALL,
            "Specify the segment count or segment numbers that you want to display for Span Creation"),
    {NULL}
};

//--------------------------------------------------------------------------
// Finally, the list of callback routines and data structures that provide
// the static hooks into our module from the other parts of the server.
//--------------------------------------------------------------------------
// Module definition for configuration.  If a particular callback is not
// needed, replace its routine name below with the word NULL.
//--------------------------------------------------------------------------
DLL_PUBLIC module AP_MODULE_DECLARE_DATA otel_apache_module =
{
    STANDARD20_MODULE_STUFF,
    ApacheConfigHandlers::otel_create_dir_config,    // per-directory config creator
    ApacheConfigHandlers::otel_merge_dir_config,     // dir config merger
    NULL,
    NULL,
    otel_cmds,                 // command table
    ApacheHooks::registerHooks // set up other request processing hooks
};

// Locate our directory configuration record for the current request.
otel_cfg* ApacheConfigHandlers::our_dconfig(const request_rec *r)
{
    return (otel_cfg *) ap_get_module_config(r->per_dir_config, &otel_apache_module);
}
