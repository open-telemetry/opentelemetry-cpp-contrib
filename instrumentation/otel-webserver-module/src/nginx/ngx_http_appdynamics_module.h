/*
* Copyright 2021 AppDynamics LLC. 
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

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <stdbool.h>
#include "../../include/core/api/AppdynamicsSdk.h"
#include "../../include/core/api/appdynamics_ngx_api.h"


#define LOWEST_HTTP_ERROR_CODE 400
/*
Function pointer struct for module specific hooks
*/
typedef ngx_int_t (*mod_handler)(ngx_http_request_t*);
mod_handler h[14];

/*
 Structure for storing module details for mapping module handlers with their respective module
*/
typedef struct {
        char* name;
        ngx_uint_t ngx_index;
        ngx_http_phases ph[2];
        mod_handler handler;
        ngx_uint_t mod_phase_index;
        ngx_uint_t phase_count;
}appd_ngx_module;

/*
	Configuration struct for module
*/
typedef struct {
    ngx_flag_t  appDynamicsEnabled;		// OPTIONAL: ON or OFF to enable the AppDynamics NGINX Agent or not respectively. (defaults to true)
    ngx_str_t	appDynamicsOtelSpanExporter;
    ngx_str_t	appDynamicsOtelExporterEndpoint;
    ngx_flag_t  appDynamicsOtelSslEnabled;
    ngx_str_t   appDynamicsOtelSslCertificatePath;
    ngx_str_t	appDynamicsOtelSpanProcessor;
    ngx_str_t	appDynamicsOtelSampler;
    ngx_str_t	appDynamicsServiceName;
    ngx_str_t   appDynamicsServiceNamespace;
    ngx_str_t   appDynamicsServiceInstanceId;
    ngx_uint_t  appDynamicsOtelMaxQueueSize;
    ngx_uint_t  appDynamicsOtelScheduledDelayMillis;
    ngx_uint_t  appDynamicsOtelExportTimeoutMillis;
    ngx_uint_t  appDynamicsOtelMaxExportBatchSize;
    ngx_flag_t  appDynamicsReportAllInstrumentedModules;
    ngx_flag_t	appDynamicsResolveBackends;
    ngx_flag_t	appDynamicsTraceAsError;
    ngx_flag_t  appDynamicsMaskCookie;
    ngx_flag_t  appDynamicsMaskSmUser;
    ngx_str_t   appDynamicsCookieMatchPattern;
    ngx_str_t   appDynamicsDelimiter;
    ngx_str_t   appDynamicsSegment;
    ngx_str_t   appDynamicsMatchfilter;
    ngx_str_t   appDynamicsMatchpattern;
    ngx_str_t   appDynamicsSegmentType;
    ngx_str_t   appDynamicsSegmentParameter;
} ngx_http_appdynamics_loc_conf_t;

/*
    Configuration structure for storing information throughout the worker process life-time
*/
typedef struct {
    ngx_flag_t  isInitialized;
    char* pid;
} ngx_http_appdynamics_worker_conf_t;

typedef struct{
    const char* key;
    const char* value;
}NGX_HTTP_APPD_RECORDS;

typedef struct {
   const char* appd_req_handle_key;
   APPD_SDK_ENV_RECORD* propagationHeaders;
   int pheaderCount;
}ngx_http_appd_handles_t;

typedef struct{
    ngx_str_t sNamespace;
    ngx_str_t sName;
    ngx_str_t sInstanceId;
}contextNode;

// static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
// static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

/* Function prototypes */
static void *ngx_http_appdynamics_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_appdynamics_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_appdynamics_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_appdynamics_init_worker(ngx_cycle_t *cycle);
static void ngx_http_appdynamics_exit_worker(ngx_cycle_t *cycle);
static ngx_flag_t ngx_initialize_appdynamics(ngx_http_request_t *r);
static void fillRequestPayload(request_payload* req_payload, ngx_http_request_t* r, int* count);
static void startMonitoringRequest(ngx_http_request_t* r);
static void stopMonitoringRequest(ngx_http_request_t* r);
static APPD_SDK_STATUS_CODE appd_startInteraction(ngx_http_request_t* r, const char* module_name);
static void appd_stopInteraction(ngx_http_request_t* r, const char* module_name);
static void appd_payload_decorator(ngx_http_request_t* r, APPD_SDK_ENV_RECORD* propagationHeaders, int count);
static ngx_flag_t appd_requestHasErrors(ngx_http_request_t* r);
static ngx_uint_t appd_getErrorCode(ngx_http_request_t* r);
static char* ngx_appd_context_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void ngx_appd_set_global_context(ngx_http_appdynamics_loc_conf_t * prev);

/*
    Module specific handler
*/
static ngx_int_t ngx_http_appd_rewrite_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_limit_conn_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_limit_req_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_realip_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_auth_request_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_auth_basic_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_access_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_static_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_gzip_static_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_dav_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_autoindex_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_index_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_random_index_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_appd_log_handler(ngx_http_request_t *r);

/*
    Utility fuction to check if the given module is monitored by Appd Agent
*/

static void traceConfig(ngx_http_request_t *r, ngx_http_appdynamics_loc_conf_t* conf);
static ngx_int_t isAppdMonitored(const char* str);
static char* computeContextName(ngx_http_request_t *r, ngx_http_appdynamics_loc_conf_t* conf);

/* Filters */
// static ngx_int_t ngx_http_appdynamics_header_filter(ngx_http_request_t *r);
// static ngx_int_t ngx_http_appdynamics_body_filter(ngx_http_request_t *r, ngx_chain_t *in);

