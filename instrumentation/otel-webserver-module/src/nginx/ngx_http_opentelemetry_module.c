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

#include "ngx_http_opentelemetry_module.h"
#include "ngx_http_opentelemetry_log.h"
#include "../../include/util/RegexResolver.h"
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

ngx_http_opentelemetry_worker_conf_t *worker_conf;
static contextNode contexts[5];
static unsigned int c_count = 0;
static unsigned int isGlobalContextSet = 0;
static ngx_str_t hostname;

/*
List of modules being monitored
*/
otel_ngx_module otel_monitored_modules[] = {
    {
        "ngx_http_realip_module",
        0,
        {NGX_HTTP_PREACCESS_PHASE},
        ngx_http_otel_realip_handler,
        0,
        2
    },
    {
        "ngx_http_rewrite_module",
        0,
        {NGX_HTTP_REWRITE_PHASE},
        ngx_http_otel_rewrite_handler,
        0,
        2
    },
    {
        "ngx_http_limit_conn_module",
        0,
        {NGX_HTTP_PREACCESS_PHASE},
        ngx_http_otel_limit_conn_handler,
        0,
        1
    },
    {
        "ngx_http_limit_req_module",
        0,
        {NGX_HTTP_PREACCESS_PHASE},
        ngx_http_otel_limit_req_handler,
        0,
        1
    },
    {
        "ngx_http_auth_request_module",
        0,
        {NGX_HTTP_ACCESS_PHASE},
        ngx_http_otel_auth_request_handler,
        0,
        1
    },
    {
        "ngx_http_auth_basic_module",
        0,
        {NGX_HTTP_ACCESS_PHASE},
        ngx_http_otel_auth_basic_handler,
        0,
        1
    },
    {
        "ngx_http_access_module",
        0,
        {NGX_HTTP_ACCESS_PHASE},
        ngx_http_otel_access_handler,
        0,
        1
    },
    {
        "ngx_http_static_module",
        0,
        {NGX_HTTP_CONTENT_PHASE},
        ngx_http_otel_static_handler,
        0,
        1
    },
    {
        "ngx_http_gzip_static_module",
        0,
        {NGX_HTTP_CONTENT_PHASE},
        ngx_http_otel_gzip_static_handler,
        0,
        1
    },
    {
        "ngx_http_dav_module",
        0,
        {NGX_HTTP_CONTENT_PHASE},
        ngx_http_otel_dav_handler,
        0,
        1
    },
    {
        "ngx_http_autoindex_module",
        0,
        {NGX_HTTP_CONTENT_PHASE},
        ngx_http_otel_autoindex_handler,
        0,
        1
    },
    {
        "ngx_http_index_module",
        0,
        {NGX_HTTP_CONTENT_PHASE},
        ngx_http_otel_index_handler,
        0,
        1
    },
    {
        "ngx_http_random_index_module",
        0,
        {NGX_HTTP_CONTENT_PHASE},
        ngx_http_otel_random_index_handler,
        0,
        1
    },
    {
        "ngx_http_log_module",
        0,
        {NGX_HTTP_LOG_PHASE},
        ngx_http_otel_log_handler,
        0,
        1
    },
    {
        "ngx_http_try_files_module",
        0,
        {NGX_HTTP_PRECONTENT_PHASE},
        ngx_http_otel_try_files_handler,
        0,
        1
    },
    {
        "ngx_http_mirror_module",
        0,
        {NGX_HTTP_PRECONTENT_PHASE},
        ngx_http_otel_mirror_handler,
        0,
        1
    }
};

/*
    List of nginx variables.
*/
static ngx_http_variable_t otel_ngx_variables[] = {
  {
    ngx_string("opentelemetry_trace_id"),
    NULL,
    ngx_opentelemetry_initialise_trace_id,
    0,
    NGX_HTTP_VAR_NOCACHEABLE | NGX_HTTP_VAR_CHANGEABLE,
    0,
  },
  {
    ngx_string("opentelemetry_span_id"),
    NULL,
    ngx_opentelemetry_initialise_span_id,
    0,
    NGX_HTTP_VAR_NOCACHEABLE | NGX_HTTP_VAR_CHANGEABLE,
    0,
  },
  {
    ngx_string("opentelemetry_context_traceparent"),
    NULL,
    ngx_opentelemetry_initialise_context_traceparent,
    0,
    NGX_HTTP_VAR_NOCACHEABLE | NGX_HTTP_VAR_CHANGEABLE,
    0,
  },
  {
    ngx_string("opentelemetry_context_b3"),
    NULL,
    ngx_opentelemetry_initialise_context_b3,
    0,
    NGX_HTTP_VAR_NOCACHEABLE | NGX_HTTP_VAR_CHANGEABLE,
    0,
  },
  
  ngx_null_command
};


/*
	Here's the list of directives specific to our module, and information about where they
	may appear and how the command parser should process them.
*/
static ngx_command_t ngx_http_opentelemetry_commands[] = {

    { ngx_string("NginxModuleEnabled"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleEnabled),
      NULL},

    { ngx_string("NginxModuleOtelSpanExporter"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleOtelSpanExporter),
      NULL},

    { ngx_string("NginxModuleOtelSslEnabled"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleOtelSslEnabled),
      NULL},

    { ngx_string("NginxModuleOtelSslCertificatePath"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleOtelSslCertificatePath),
      NULL},

    { ngx_string("NginxModuleOtelExporterEndpoint"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleOtelExporterEndpoint),
      NULL},

    { ngx_string("NginxModuleOtelExporterOtlpHeaders"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleOtelExporterOtlpHeaders),
      NULL},

    { ngx_string("NginxModuleOtelSpanProcessor"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleOtelSpanProcessor),
      NULL},

    { ngx_string("NginxModuleOtelSampler"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleOtelSampler),
      NULL},

    { ngx_string("NginxModuleServiceName"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleServiceName),
      NULL},

    { ngx_string("NginxModuleServiceNamespace"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleServiceNamespace),
      NULL},

    { ngx_string("NginxModuleServiceInstanceId"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleServiceInstanceId),
      NULL},

    { ngx_string("NginxModuleOtelMaxQueueSize"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleOtelMaxQueueSize),
      NULL},

    { ngx_string("NginxModuleOtelScheduledDelayMillis"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleOtelScheduledDelayMillis),
      NULL},

    { ngx_string("NginxModuleOtelExportTimeoutMillis"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleOtelExportTimeoutMillis),
      NULL},

    { ngx_string("NginxModuleOtelMaxExportBatchSize"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleOtelMaxExportBatchSize),
      NULL},

    { ngx_string("NginxModuleResolveBackends"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleResolveBackends),
      NULL},

    { ngx_string("NginxModuleTraceAsError"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleTraceAsError),
      NULL},

    { ngx_string("NginxModuleReportAllInstrumentedModules"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleReportAllInstrumentedModules),
      NULL},

    { ngx_string("NginxModuleWebserverContext"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE3,
      ngx_otel_context_set,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL},

    { ngx_string("NginxModuleMaskCookie"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleMaskCookie),
      NULL},

    { ngx_string("NginxModuleCookieMatchPattern"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleCookieMatchPattern),
      NULL},

    { ngx_string("NginxModuleMaskSmUser"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleMaskSmUser),
      NULL},

    { ngx_string("NginxModuleDelimiter"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleDelimiter),
      NULL},

    { ngx_string("NginxModuleSegment"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleSegment),
      NULL},

    { ngx_string("NginxModuleMatchfilter"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleMatchfilter),
      NULL},

    { ngx_string("NginxModuleMatchpattern"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleMatchpattern),
      NULL},

    { ngx_string("NginxModuleSegmentType"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleSegmentType),
      NULL},

    { ngx_string("NginxModuleSegmentParameter"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleSegmentParameter),
      NULL},

    { ngx_string("NginxModuleRequestHeaders"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleRequestHeaders),
      NULL},

    { ngx_string("NginxModuleResponseHeaders"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleResponseHeaders),
      NULL},
    
    { ngx_string("NginxModuleTrustIncomingSpans"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleTrustIncomingSpans),
      NULL},
    	
    { ngx_string("NginxModuleAttributes"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_otel_attributes_set,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL},

    { ngx_string("NginxModuleIgnorePaths"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_ignore_path_set,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL},
    
    { ngx_string("NginxModulePropagatorType"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_propagator,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModulePropagatorType),
      NULL},
    
    { ngx_string("NginxModuleOperationName"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_opentelemetry_loc_conf_t, nginxModuleOperationName),
      NULL},
    
    /* command termination */
    ngx_null_command
};


/* The module context. */
static ngx_http_module_t ngx_http_opentelemetry_module_ctx = {
    ngx_http_opentelemetry_create_variables,		/* preconfiguration */
    ngx_http_opentelemetry_init,	                /* postconfiguration */

    NULL,	                                        /* create main configuration */
    NULL,	                                        /* init main configuration */

    NULL,	                                        /* create server configuration */
    NULL,	                                        /* merge server configuration */

    ngx_http_opentelemetry_create_loc_conf,	        /* create location configuration */
    ngx_http_opentelemetry_merge_loc_conf	        /* merge location configuration */
};

/* Module definition. */
ngx_module_t ngx_http_opentelemetry_module = {
    NGX_MODULE_V1,							/* module version and a signature */
    &ngx_http_opentelemetry_module_ctx,		                        /* module context */
    ngx_http_opentelemetry_commands,			                /* module directives */
    NGX_HTTP_MODULE, 						        /* module type */
    NULL, 								/* init master */
    NULL, 								/* init module */
    ngx_http_opentelemetry_init_worker, 	                                /* init process */
    NULL, 								/* init thread */
    NULL, 								/* exit thread */
    ngx_http_opentelemetry_exit_worker,   				/* exit process */
    NULL, 								/* exit master */
    NGX_MODULE_V1_PADDING
};

/*
	Create loc conf to be used by the module
	It takes a directive struct (ngx_conf_t) and returns a newly
	created module configuration struct
 */
static void* ngx_http_opentelemetry_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_opentelemetry_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_opentelemetry_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }

    /* Initialize */
    conf->nginxModuleEnabled                   = NGX_CONF_UNSET;
    conf->nginxModuleResolveBackends           = NGX_CONF_UNSET;
    conf->nginxModuleOtelScheduledDelayMillis  = NGX_CONF_UNSET;
    conf->nginxModuleOtelExportTimeoutMillis   = NGX_CONF_UNSET;
    conf->nginxModuleOtelMaxExportBatchSize    = NGX_CONF_UNSET;
    conf->nginxModuleReportAllInstrumentedModules = NGX_CONF_UNSET;
    conf->nginxModuleMaskCookie                = NGX_CONF_UNSET;
    conf->nginxModuleMaskSmUser                = NGX_CONF_UNSET;
    conf->nginxModuleTraceAsError              = NGX_CONF_UNSET;
    conf->nginxModuleOtelMaxQueueSize          = NGX_CONF_UNSET;
    conf->nginxModuleOtelSslEnabled            = NGX_CONF_UNSET;
    conf->nginxModuleTrustIncomingSpans              = NGX_CONF_UNSET;

    return conf;
}

static char* ngx_http_opentelemetry_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_opentelemetry_loc_conf_t *prev = parent;
    ngx_http_opentelemetry_loc_conf_t *conf = child;
    ngx_otel_set_global_context(prev);
    ngx_otel_set_attributes(prev, conf);
    ngx_conf_merge_ignore_paths(prev, conf);

    ngx_conf_merge_value(conf->nginxModuleEnabled, prev->nginxModuleEnabled, 1);
    ngx_conf_merge_value(conf->nginxModuleReportAllInstrumentedModules, prev->nginxModuleReportAllInstrumentedModules, 0);
    ngx_conf_merge_value(conf->nginxModuleResolveBackends, prev->nginxModuleResolveBackends, 1);
    ngx_conf_merge_value(conf->nginxModuleTraceAsError, prev->nginxModuleTraceAsError, 0);
    ngx_conf_merge_value(conf->nginxModuleMaskCookie, prev->nginxModuleMaskCookie, 0);
    ngx_conf_merge_value(conf->nginxModuleMaskSmUser, prev->nginxModuleMaskSmUser, 0);
    ngx_conf_merge_value(conf->nginxModuleTrustIncomingSpans, prev->nginxModuleTrustIncomingSpans, 1);


    ngx_conf_merge_str_value(conf->nginxModuleOtelSpanExporter, prev->nginxModuleOtelSpanExporter, "");
    ngx_conf_merge_str_value(conf->nginxModuleOtelExporterEndpoint, prev->nginxModuleOtelExporterEndpoint, "");
    ngx_conf_merge_str_value(conf->nginxModuleOtelExporterOtlpHeaders, prev->nginxModuleOtelExporterOtlpHeaders, "");
    ngx_conf_merge_value(conf->nginxModuleOtelSslEnabled, prev->nginxModuleOtelSslEnabled, 0);
    ngx_conf_merge_str_value(conf->nginxModuleOtelSslCertificatePath, prev->nginxModuleOtelSslCertificatePath, "");
    ngx_conf_merge_str_value(conf->nginxModuleOtelSpanProcessor, prev->nginxModuleOtelSpanProcessor, "");
    ngx_conf_merge_str_value(conf->nginxModuleOtelSampler, prev->nginxModuleOtelSampler, "");
    ngx_conf_merge_str_value(conf->nginxModuleServiceName, prev->nginxModuleServiceName, "");
    ngx_conf_merge_str_value(conf->nginxModuleServiceNamespace, prev->nginxModuleServiceNamespace, "");
    ngx_conf_merge_str_value(conf->nginxModuleServiceInstanceId, prev->nginxModuleServiceInstanceId, "");
    ngx_conf_merge_str_value(conf->nginxModuleCookieMatchPattern, prev->nginxModuleCookieMatchPattern, "");
    ngx_conf_merge_str_value(conf->nginxModuleDelimiter, prev->nginxModuleDelimiter, "");
    ngx_conf_merge_str_value(conf->nginxModuleMatchfilter, prev->nginxModuleMatchfilter, "");
    ngx_conf_merge_str_value(conf->nginxModuleSegment, prev->nginxModuleSegment, "");
    ngx_conf_merge_str_value(conf->nginxModuleMatchpattern, prev->nginxModuleMatchpattern, "");

    ngx_conf_merge_size_value(conf->nginxModuleOtelMaxQueueSize, prev->nginxModuleOtelMaxQueueSize, 2048);
    ngx_conf_merge_msec_value(conf->nginxModuleOtelScheduledDelayMillis, prev->nginxModuleOtelScheduledDelayMillis, 5000);
    ngx_conf_merge_msec_value(conf->nginxModuleOtelExportTimeoutMillis, prev->nginxModuleOtelExportTimeoutMillis, 30000);
    ngx_conf_merge_size_value(conf->nginxModuleOtelMaxExportBatchSize, prev->nginxModuleOtelMaxExportBatchSize, 512);

    ngx_conf_merge_str_value(conf->nginxModuleSegmentType, prev->nginxModuleSegmentType, "First");
    ngx_conf_merge_str_value(conf->nginxModuleSegmentParameter, prev->nginxModuleSegmentParameter, "2");
    ngx_conf_merge_str_value(conf->nginxModuleRequestHeaders, prev->nginxModuleRequestHeaders, "");
    ngx_conf_merge_str_value(conf->nginxModuleResponseHeaders, prev->nginxModuleResponseHeaders, "");
    ngx_conf_merge_str_value(conf->nginxModulePropagatorType, prev->nginxModulePropagatorType, "w3c");
    ngx_conf_merge_str_value(conf->nginxModuleOperationName, prev->nginxModuleOperationName, "");

    return NGX_CONF_OK;
}

/*
	Function to initialize the module and used to register all the phases handlers and filters.
	-------------------------------------------------------------------------------------------------
	For reference: HTTP Request phases

	Each HTTP request passes through a sequence of phases. In each phase a distinct type of processing
	is performed on the request. Module-specific handlers can be registered in most phases, and many
	standard nginx modules register their phase handlers as a way to get called at a specific stage of
	request processing. Phases are processed successively and the phase handlers are called once the
	request reaches the phase. Following is the list of nginx HTTP phases:

	NGX_HTTP_POST_READ_PHASE
	NGX_HTTP_SERVER_REWRITE_PHASE
	NGX_HTTP_FIND_CONFIG_PHASE
	NGX_HTTP_REWRITE_PHASE
	NGX_HTTP_POST_REWRITE_PHASE
	NGX_HTTP_PREACCESS_PHASE
	NGX_HTTP_ACCESS_PHASE
	NGX_HTTP_POST_ACCESS_PHASE
	NGX_HTTP_PRECONTENT_PHASE
	NGX_HTTP_CONTENT_PHASE
	NGX_HTTP_LOG_PHASE

	On every phase you can register any number of your handlers. Exceptions are following phases:

	NGX_HTTP_FIND_CONFIG_PHASE
	NGX_HTTP_POST_ACCESS_PHASE
	NGX_HTTP_POST_REWRITE_PHASE
	NGX_HTTP_TRY_FILES_PHASE
	-------------------------------------------------------------------------------------------------
 */
static ngx_int_t ngx_http_opentelemetry_create_variables(ngx_conf_t *cf){
    for (ngx_http_variable_t* v = otel_ngx_variables; v->name.len; v++) {
        ngx_http_variable_t* var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }
        var->get_handler = v->get_handler;
        var->set_handler = v->set_handler;
        var->data = v->data;
        v->index = var->index = ngx_http_get_variable_index(cf, &v->name);
    }

    return NGX_OK;
}

ngx_int_t ngx_opentelemetry_initialise_trace_id(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data) {
    ngx_http_otel_handles_t* ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);

    if(ctx->trace_id.len){
        v->len = ctx->trace_id.len;
        v->data = ctx->trace_id.data;
    }else{
        v->len = 0;
        v->data = "";
    }
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}

ngx_int_t ngx_opentelemetry_initialise_span_id(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data) {
    ngx_http_otel_handles_t* ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);

    if(ctx->root_span_id.len){
        v->len = ctx->root_span_id.len;
        v->data = ctx->root_span_id.data;
    }else{
        v->len = 0;
        v->data = "";
    }
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}

ngx_int_t ngx_opentelemetry_initialise_context_traceparent(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data) {
    ngx_http_otel_handles_t* ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    ngx_http_opentelemetry_loc_conf_t *conf = ngx_http_get_module_loc_conf(r, ngx_http_opentelemetry_module);
    ngx_str_t propagator_type = conf->nginxModulePropagatorType;
    if(ctx->tracing_context.len && !strcmp(propagator_type.data, "w3c")){
        v->len = ctx->tracing_context.len;
        v->data = ctx->tracing_context.data;
    }else{
        v->len = 0;
        v->data = "";
    }
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    
    return NGX_OK;
}

ngx_int_t ngx_opentelemetry_initialise_context_b3(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data) {
    ngx_http_otel_handles_t* ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    ngx_http_opentelemetry_loc_conf_t *conf = ngx_http_get_module_loc_conf(r, ngx_http_opentelemetry_module);
    ngx_str_t propagator_type = conf->nginxModulePropagatorType;
    if(ctx->tracing_context.len && !strcmp(propagator_type.data, "b3")){
        v->len = ctx->tracing_context.len;
        v->data = ctx->tracing_context.data;
    }else{
        v->len = 0;
        v->data = "";
    }
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}

static ngx_int_t ngx_http_opentelemetry_init(ngx_conf_t *cf)
{
    ngx_http_core_main_conf_t    *cmcf;
    ngx_uint_t                   m, cp, ap, pap, srp, prp, rp, lp, pcp;
    ngx_http_phases              ph;
    ngx_uint_t                   phase_index;
    ngx_int_t                    res;

    ngx_writeError(cf->cycle->log, __func__, "Starting Opentelemetry Module init");

    cp = ap = pap = srp = prp = rp = lp = pcp = 0;

    res = -1;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    ngx_writeError(cf->cycle->log, __func__, "Registering handlers for modules in different phases");

    for (m = 0; cf->cycle->modules[m]; m++) {
        if (cf->cycle->modules[m]->type == NGX_HTTP_MODULE) {
            res = isOTelMonitored(cf->cycle->modules[m]->name);
            if(res != -1){
                otel_monitored_modules[res].ngx_index = m;
                phase_index = otel_monitored_modules[res].mod_phase_index;
                while(phase_index < otel_monitored_modules[res].phase_count){
                    ph = otel_monitored_modules[res].ph[phase_index];
                    switch(ph){
                        case NGX_HTTP_POST_READ_PHASE:
                            if(prp < cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers.nelts){
                                h[res] = ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers.elts)[prp];
                                ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers.elts)[prp] = otel_monitored_modules[res].handler;
                                prp++;
                            }
                            break;

                        case NGX_HTTP_SERVER_REWRITE_PHASE:
                            if(srp < cmcf->phases[NGX_HTTP_SERVER_REWRITE_PHASE].handlers.nelts){
                                h[res] = ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_SERVER_REWRITE_PHASE].handlers.elts)[srp];
                                ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_SERVER_REWRITE_PHASE].handlers.elts)[srp] = otel_monitored_modules[res].handler;
                                srp++;
                            }
                            break;

                        case NGX_HTTP_REWRITE_PHASE:
                            if(rp < cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers.nelts){
                                h[res] = ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers.elts)[rp];
                                ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers.elts)[rp] = otel_monitored_modules[res].handler;
                                rp++;
                            }
                            break;

                        case NGX_HTTP_PREACCESS_PHASE:
                            if(pap < cmcf->phases[NGX_HTTP_PREACCESS_PHASE].handlers.nelts){
                                h[res] = ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_PREACCESS_PHASE].handlers.elts)[pap];
                                ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_PREACCESS_PHASE].handlers.elts)[pap] = otel_monitored_modules[res].handler;
                                pap++;
                            }
                            break;

                        case NGX_HTTP_ACCESS_PHASE:
                            if(ap < cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers.nelts){
                                h[res] = ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers.elts)[ap];
                                ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers.elts)[ap] = otel_monitored_modules[res].handler;
                                ap++;
                            }
                            break;

                        case NGX_HTTP_CONTENT_PHASE:
                            if(cp < cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers.nelts){
                                h[res] = ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers.elts)[cp];
                                ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers.elts)[cp] = otel_monitored_modules[res].handler;
                                cp++;
                            }
                            break;

                        case NGX_HTTP_LOG_PHASE:
                            if(lp < cmcf->phases[NGX_HTTP_LOG_PHASE].handlers.nelts){
                                h[res] = ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_LOG_PHASE].handlers.elts)[lp];
                                ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_LOG_PHASE].handlers.elts)[cp] = otel_monitored_modules[res].handler;
                                lp++;
                            }
                            break;
                        case NGX_HTTP_PRECONTENT_PHASE:
                            if(pcp < cmcf->phases[NGX_HTTP_PRECONTENT_PHASE].handlers.nelts){
                                h[res] = ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_PRECONTENT_PHASE].handlers.elts)[pcp];
                                ((ngx_http_handler_pt*)cmcf->phases[NGX_HTTP_PRECONTENT_PHASE].handlers.elts)[pcp] = otel_monitored_modules[res].handler;
                                pcp++;
                            }
                            break;
                        case NGX_HTTP_FIND_CONFIG_PHASE:
                        case NGX_HTTP_POST_REWRITE_PHASE:
                        case NGX_HTTP_POST_ACCESS_PHASE:
                            break;
                    }
                    phase_index++;
                }
            }
        }
    }

	/* Register header_filter */
    // ngx_http_next_header_filter = ngx_http_top_header_filter;
    // ngx_http_top_header_filter = ngx_http_opentelemetry_header_filter;

    /* Register body_filter */
    // ngx_http_next_body_filter = ngx_http_top_body_filter;
    // ngx_http_top_body_filter = ngx_http_opentelemetry_body_filter;

    hostname = cf->cycle->hostname;
    /* hostname is extracted from the nginx cycle. The attribute hostname is needed
    for OTEL spec and the only place it is available is cf->cycle
    */
    ngx_writeError(cf->cycle->log, __func__, "Opentelemetry Module init completed!");

  return NGX_OK;
}

/*
    This function gets called when master process creates worker processes
*/
static ngx_int_t ngx_http_opentelemetry_init_worker(ngx_cycle_t *cycle)
{
    int p = getpid();
    char * s = (char *)ngx_pcalloc(cycle->pool, 6);
    sprintf(s, "%d", p);
    ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "mod_opentelemetry: ngx_http_opentelemetry_init_worker: Initializing Nginx Worker for process with PID: %s", s);

    /* Allocate memory for worker configuration */
    worker_conf = ngx_pcalloc(cycle->pool, sizeof(ngx_http_opentelemetry_worker_conf_t));
    if (worker_conf == NULL) {
       ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "mod_opentelemetry: ngx_http_opentelemetry_init_worker: Not able to allocate memeory for worker conf");
       return NGX_ERROR;
    }

    worker_conf->pid = s;

    return NGX_OK;
}

/*
    This function gets called when a worker process pool is destroyed
*/
static void ngx_http_opentelemetry_exit_worker(ngx_cycle_t *cycle)
{
    if (worker_conf && worker_conf->isInitialized)
    {
        ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "mod_opentelemetry: ngx_http_opentelemetry_exit_worker: Exiting Nginx Worker for process with PID: %s**********", worker_conf->pid);
    }
}

static char* ngx_otel_attributes_set(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) {
    ngx_http_opentelemetry_loc_conf_t * my_conf=(ngx_http_opentelemetry_loc_conf_t *)conf;

    ngx_str_t *value = cf->args->elts;

    ngx_array_t *arr;
    ngx_str_t   *elt;
    ngx_int_t arg_count = cf->args->nelts; 

    arr = ngx_array_create(cf->pool, arg_count, sizeof(ngx_str_t));

    if (arr == NULL) {
        return NGX_CONF_ERROR;
    }

    // Add elements to the array
    for (ngx_int_t i = 1; i < arg_count; i++) {
        elt = ngx_array_push(arr);
        if (elt == NULL) {
            return NGX_CONF_ERROR;
        }
        ngx_str_set(elt, value[i].data);
        elt->len = ngx_strlen(value[i].data);
    }
    my_conf->nginxModuleAttributes = arr;
    return NGX_CONF_OK;

}

static char* ngx_conf_set_propagator(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) {
    ngx_http_opentelemetry_loc_conf_t * my_conf=(ngx_http_opentelemetry_loc_conf_t *)conf;

    ngx_str_t *value = cf->args->elts;
    ngx_str_t elt;
    if( !strcmp(value[1].data, "b3") || !strcmp(value[1].data, "B3") ){
        elt.data = (u_char *)"b3";
        elt.len = sizeof("b3") - 1;
        my_conf->nginxModulePropagatorType = elt;
    }
    else{
        elt.data = (u_char *)"w3c";
        elt.len = sizeof("w3c") - 1;
        my_conf->nginxModulePropagatorType = elt;
    }
    return NGX_CONF_OK;

}
static char* ngx_conf_ignore_path_set(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) {
    ngx_http_opentelemetry_loc_conf_t * my_conf=(ngx_http_opentelemetry_loc_conf_t *)conf;

    ngx_str_t *value = cf->args->elts;
    ngx_array_t *arr;
    ngx_str_t   *elt;
    ngx_int_t arg_count = cf->args->nelts; 

    arr = ngx_array_create(cf->pool, arg_count, sizeof(ngx_str_t));
    if (arr == NULL) {
        return NGX_CONF_ERROR;
    }

    for (ngx_int_t i = 1; i < arg_count; i++) {
        elt = ngx_array_push(arr);
        if (elt == NULL) {
            return NGX_CONF_ERROR;
        }
        ngx_str_set(elt, value[i].data);
        elt->len = ngx_strlen(value[i].data);
    }
    my_conf->nginxModuleIgnorePaths = arr;
    return NGX_CONF_OK;

}

static char* ngx_otel_context_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
    ngx_str_t* value;

    value = cf->args->elts;
    ngx_http_opentelemetry_loc_conf_t * otel_conf_temp=(ngx_http_opentelemetry_loc_conf_t *)conf;
    if(cf->args->nelts == 4){
        contexts[c_count].sNamespace = value[1];
        otel_conf_temp->nginxModuleServiceNamespace = value[1];
        contexts[c_count].sName = value[2];
        otel_conf_temp->nginxModuleServiceName = value[2];
        contexts[c_count].sInstanceId = value[3];
        otel_conf_temp->nginxModuleServiceInstanceId = value[3];
        c_count++;
    }

    return NGX_CONF_OK;
}
static void ngx_otel_set_global_context(ngx_http_opentelemetry_loc_conf_t * prev)
{
    if(isGlobalContextSet==0){
      if((prev->nginxModuleServiceName).data != NULL && (prev->nginxModuleServiceNamespace).data != NULL && (prev->nginxModuleServiceInstanceId).data != NULL){
        isGlobalContextSet = 1;
        contexts[c_count].sNamespace = prev->nginxModuleServiceNamespace;
        contexts[c_count].sName = prev->nginxModuleServiceName;
        contexts[c_count].sInstanceId = prev->nginxModuleServiceInstanceId;
        c_count++;
      }
    }
}

static void ngx_otel_set_attributes(ngx_http_opentelemetry_loc_conf_t * prev, ngx_http_opentelemetry_loc_conf_t * conf)
{
    if (conf->nginxModuleAttributes && (conf->nginxModuleAttributes->nelts) > 0) {
        return;
    }
    if (prev->nginxModuleAttributes && (prev->nginxModuleAttributes->nelts) > 0) {
        if((conf->nginxModuleAttributes) == NULL)
        {
            conf->nginxModuleAttributes = prev->nginxModuleAttributes;
        }
    }
    return;
}

static void ngx_conf_merge_ignore_paths(ngx_http_opentelemetry_loc_conf_t * prev, ngx_http_opentelemetry_loc_conf_t * conf)
{
    if (conf->nginxModuleIgnorePaths && (conf->nginxModuleIgnorePaths->nelts) > 0) {
        return;
    }
    if (prev->nginxModuleIgnorePaths && (prev->nginxModuleIgnorePaths->nelts) > 0) {
        if((conf->nginxModuleIgnorePaths) == NULL)
        {
            conf->nginxModuleIgnorePaths = prev->nginxModuleIgnorePaths;
        }
    }
    return;
}

/*
    Begin a new interaction
*/
static OTEL_SDK_STATUS_CODE otel_startInteraction(ngx_http_request_t* r, const char* module_name){
    OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;
    ngx_http_otel_handles_t* ctx;

    if(!r || r->internal)
    {
        ngx_writeTrace(r->connection->log, __func__, "It is not a main Request, not starting interaction");
        return res;
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    if(ctx && ctx->otel_req_handle_key)
    {
        ngx_flag_t resolveBackends = false;
        ngx_http_opentelemetry_loc_conf_t* conf;
        conf = ngx_http_get_module_loc_conf(r, ngx_http_opentelemetry_module);
        if(conf)
        {
            resolveBackends = conf->nginxModuleResolveBackends;
        }
        OTEL_SDK_ENV_RECORD* propagationHeaders = ngx_pcalloc(r->pool, ALL_PROPAGATION_HEADERS_COUNT * sizeof(OTEL_SDK_ENV_RECORD));
        if (propagationHeaders == NULL)
        {
            ngx_writeError(r->connection->log, __func__, "Failed to allocate memory for propagation headers");
            return OTEL_STATUS(fail);
        }
        ngx_writeTrace(r->connection->log, __func__, "Starting a new module interaction for: %s", module_name);
        int ix = 0;
        res = startModuleInteraction((void*)ctx->otel_req_handle_key, module_name, "", resolveBackends, propagationHeaders, &ix);


        if (OTEL_ISSUCCESS(res))
        {
            removeUnwantedHeader(r);
            otel_payload_decorator(r, propagationHeaders, ix);
            otel_variables_decorator(r);
            ngx_writeTrace(r->connection->log, __func__, "Interaction begin successful");
        }
        else
        {
            ngx_writeError(r->connection->log, __func__, "Error: Interaction begin result code: %d", res);
        }
        for(int i=0;i<ix;i++)
        {
          if(propagationHeaders[i].name)
            free(propagationHeaders[i].name);
          if(propagationHeaders[i].value)
            free(propagationHeaders[i].value);
        }
    }
    return res;
}

/*
Function otel_variables_decorator is used to fill the values for Nginx tracing info variables like opentelemetry_trace_id,
opentelemetry_span_id, opentelemetry_context_b3, opentelemetry_context_traceparent. It fills the information into
ctx (module context), which is then later used by the getter function of the given nginx variables.
*/
static void otel_variables_decorator(ngx_http_request_t* r){
    ngx_str_t trace_id, span_id, tracing_context;
    ngx_list_part_t  *part;
    ngx_table_elt_t  *header;
    ngx_table_elt_t  *h;
    ngx_uint_t       nelts;

    ngx_http_otel_handles_t* ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    ngx_http_opentelemetry_loc_conf_t *conf = ngx_http_get_module_loc_conf(r, ngx_http_opentelemetry_module);
    ngx_str_t propagator_type = conf->nginxModulePropagatorType;
    part = &r->headers_in.headers.part;
    header = (ngx_table_elt_t*)part->elts;
    nelts = part->nelts;
    if(ctx->trace_id.len == 0){
        // Getting value of trace_id from the request headers.
        if(!strcmp(propagator_type.data, "b3")){
            for(ngx_uint_t j = 0; j<nelts; j++){
                h = &header[j];
                if(strcasecmp("x-b3-traceid", h->key.data)==0){
                    trace_id.data = h->value.data;
                    trace_id.len = h->value.len;

                    ctx->trace_id.data = trace_id.data;
                    ctx->trace_id.len = trace_id.len;
                }
            }
        }else{
            for(ngx_uint_t j = 0; j<nelts; j++){
                h = &header[j];
                if(strcasecmp("traceparent", h->key.data)==0){
                    u_char *temp_trace_id = ngx_pnalloc(r->pool, TRACE_ID_LEN + 1);
                    ngx_memcpy(temp_trace_id, h->value.data + 3, TRACE_ID_LEN);
                    temp_trace_id[TRACE_ID_LEN] = '\0';
                    trace_id.data = temp_trace_id;
                    trace_id.len = TRACE_ID_LEN;

                    ctx->trace_id.data = trace_id.data;
                    ctx->trace_id.len = trace_id.len;
                }
            }
        }
    }

    if(ctx->root_span_id.len == 0){
        // Getting root span id fromm ctx->propagationHeaders which is filled during otel_payload_decorator() call.
        for(int i = 0 ; i < ctx->pheaderCount ; i++ ){
            if(strcasecmp(ctx->propagationHeaders[i].name , "Parent_Span_Id") == 0){
                ctx->root_span_id.data = ngx_pcalloc(r->pool, strlen(ctx->propagationHeaders[i].value));
                ngx_memcpy(ctx->root_span_id.data, ctx->propagationHeaders[i].value, strlen(ctx->propagationHeaders[i].value));
                ctx->root_span_id.len = strlen(ctx->propagationHeaders[i].value);
            }
        }
    }
    
    if(ctx->tracing_context.len == 0){
        // Constructing the complete trace context for w3c or b3 headers.
        if(!strcmp(propagator_type.data, "w3c")){
            for(ngx_uint_t j = 0; j<nelts; j++){
                h = &header[j];
                if(strcasecmp("traceparent", h->key.data)==0){
                    ctx->tracing_context.data = ngx_pcalloc(r->pool, h->value.len + 1);
                    ngx_memcpy(ctx->tracing_context.data, h->value.data, h->value.len + 1);
                    ngx_memcpy(ctx->tracing_context.data + TRACE_ID_LEN + 4, ctx->root_span_id.data , SPAN_ID_LEN);
                    // We are trying to replace span_id in the traceparent context, so we need to start after version 
                    // and trace_id in the traceparent context
                    ctx->tracing_context.len = h->value.len;
                }
            }
        }
        else if(!strcmp(propagator_type.data, "b3")){
            ngx_str_t sampled;
            ngx_uint_t has_trace_id = 0, has_span_id = 0 , has_sampled = 0;

            for(ngx_uint_t j = 0; j<nelts; j++){
                h = &header[j];
                if(strcasecmp("x-b3-sampled", h->key.data)==0){
                    has_sampled = 1;
                    sampled.data = h->value.data;
                    sampled.len = h->value.len;
                }
            }
            if(ctx->root_span_id.len != 0){
                has_span_id = 1;
            }
            if(ctx->trace_id.len != 0){
                has_trace_id = 1;
            }
            if(has_trace_id && has_span_id){
                size_t total_length = TRACE_ID_LEN + SPAN_ID_LEN + 1;
                ngx_str_t result;
                if(has_sampled){
                    total_length+=2;
                    // length of "sampled" field and a '-'
                }
                ctx->tracing_context.data = ngx_pcalloc(r->pool, total_length+1);
                u_char *p = ctx->tracing_context.data;
                p = ngx_copy(p, ctx->trace_id.data, ctx->trace_id.len);
                p = ngx_copy(p, "-", 1);
                p = ngx_copy(p, ctx->root_span_id.data, ctx->root_span_id.len);
                if(has_sampled){ 
                    p = ngx_copy(p, "-", 1);
                    p = ngx_copy(p, sampled.data, sampled.len); 
                }
                p[total_length] = '\0';
                ctx->tracing_context.len = total_length;
            }
        }
    }
}

static void otel_payload_decorator(ngx_http_request_t* r, OTEL_SDK_ENV_RECORD* propagationHeaders, int count)
{
    ngx_list_part_t  *part;
    ngx_table_elt_t  *header;
    ngx_table_elt_t            *h;
    ngx_http_header_t          *hh;
    ngx_http_core_main_conf_t  *cmcf;

    part = &r->headers_in.headers.part;
    header = (ngx_table_elt_t*)part->elts;

    for(int i=0; i<count; i++){

        int header_found=0;
        int prop_index = -1;
        for(ngx_uint_t j = 0;; j++){
            if (j >= part->nelts) {
                if (part->next == NULL) {
                    break;
                }
                part = part->next;
                header = (ngx_table_elt_t*)part->elts;
                j = 0;
            }
            h = &header[j];
            if(strcasecmp(propagationHeaders[i].name, h->key.data)==0){
                
                header_found=1;

                if(h->key.data)
                        ngx_pfree(r->pool, h->key.data);
                if(h->value.data)
                        ngx_pfree(r->pool, h->value.data);
                
                break;
            }
        }
        if(header_found==0)
        {
            h = ngx_list_push(&r->headers_in.headers);
        }

        if(h == NULL )
                return;

        h->key.len = strlen(propagationHeaders[i].name);
        h->key.data = ngx_pcalloc(r->pool, sizeof(char)*((h->key.len)+1));
        strcpy(h->key.data, propagationHeaders[i].name);

        ngx_writeTrace(r->connection->log, __func__, "Key : %s", propagationHeaders[i].name);

        h->hash = ngx_hash_key(h->key.data, h->key.len);

        h->value.len = strlen(propagationHeaders[i].value);
        h->value.data = ngx_pcalloc(r->pool, sizeof(char)*((h->value.len)+1));
        strcpy(h->value.data, propagationHeaders[i].value);
        h->lowcase_key = h->key.data;

        cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
        hh = ngx_hash_find(&cmcf->headers_in_hash, h->hash,h->lowcase_key, h->key.len);
        if (hh && hh->handler(r, h, hh->offset) != NGX_OK) {
            return;
        }

        ngx_writeTrace(r->connection->log, __func__, "Value : %s", propagationHeaders[i].value);

    }
    
    ngx_http_otel_handles_t* ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    ctx->propagationHeaders = propagationHeaders;
    ctx->pheaderCount = count;
}

/*
    Stopping an Interaction
*/
static void otel_stopInteraction(ngx_http_request_t* r, const char* module_name,
    void* request_handle_key)
{
    if(!r || r->internal)
    {
        return;
    }

    OTEL_SDK_HANDLE_REQ otel_req_handle_key = OTEL_SDK_NO_HANDLE;
    ngx_http_otel_handles_t* ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    if (r->pool == NULL && request_handle_key != NULL)
    {
        otel_req_handle_key = request_handle_key;
    }
    else if (ctx && ctx->otel_req_handle_key)
    {
        otel_req_handle_key = ctx->otel_req_handle_key;
    }
    else
    {
        return;
    }

    // TODO: Work on backend naming and type
    char* backendName = (char *)malloc(6 * sizeof(char));
    *backendName = '\0';
    const char* backendType = "HTTP";
    unsigned int errCode=0;
    char* code = (char *)malloc(6 * sizeof(char));

    const char* status = "HTTP Status Code: ";
    char* msg = (char *)malloc(strlen(status) + 6 * sizeof(char));
    *msg = '\0';
    if(otel_requestHasErrors(r))
    {
        errCode=(unsigned int)otel_getErrorCode(r);
        sprintf(code, "%d", errCode);
        strcpy(msg, status);
        strcat(msg, code);
    }
    ngx_writeTrace(r->connection->log, __func__, "Stopping the Interaction for: %s", module_name);
    OTEL_SDK_STATUS_CODE res = stopModuleInteraction(otel_req_handle_key, backendName, backendType, errCode, msg);
    if (OTEL_ISFAIL(res))
    {
        ngx_writeError(r->connection->log, __func__, "Error: Stop Interaction failed, result code: %d", res);
    }

    free (backendName);
    free (code);
    free (msg);
}

static ngx_flag_t otel_requestHasErrors(ngx_http_request_t* r)
{
    return (r->err_status >= LOWEST_HTTP_ERROR_CODE)||(r->headers_out.status >= LOWEST_HTTP_ERROR_CODE);
}
static ngx_uint_t otel_getErrorCode(ngx_http_request_t* r)
{
    if(r->err_status >= LOWEST_HTTP_ERROR_CODE)
      return r->err_status;
    else if(r->headers_out.status >= LOWEST_HTTP_ERROR_CODE)
      return r->headers_out.status;
    else return 0;
}

static void resolve_attributes_variables(ngx_http_request_t* r)
{
    ngx_http_opentelemetry_loc_conf_t *conf = ngx_http_get_module_loc_conf(r, ngx_http_opentelemetry_module);

    for (ngx_uint_t j = 0; j < conf->nginxModuleAttributes->nelts; j++) {
        
        void *element = conf->nginxModuleAttributes->elts + (j * conf->nginxModuleAttributes->size);
        ngx_str_t var_name = (((ngx_str_t *)(conf->nginxModuleAttributes->elts))[j]);
        ngx_uint_t           key; // The variable's hashed key.
        ngx_http_variable_value_t  *value; // Pointer to the value object.

        if(var_name.data[0] == NGINX_VARIABLE_IDENTIFIER){
            // Get the hashed key.
            ngx_str_t new_var_name = var_name;
            new_var_name.data++;
            new_var_name.len--;
            key = ngx_hash_key(new_var_name.data, new_var_name.len);

            // Get the variable.
            value = ngx_http_get_variable(r, &new_var_name, key);

            if (value == NULL || value->not_found) {
                // Variable was not found.
            } else {
                ngx_str_t * ngx_str = (ngx_str_t *)(element);
                ngx_str->data = value->data;
                ngx_str->len = value->len;
                // Variable was found, `value` now contains the value.
            }
        }
    }
}

static ngx_flag_t check_ignore_paths(ngx_http_request_t *r)
{
    ngx_http_opentelemetry_loc_conf_t * conf = ngx_http_get_module_loc_conf(r, ngx_http_opentelemetry_module);
    
    ngx_uint_t uriLen =  r->uri.len;
    char *pathToCheck = ngx_pnalloc(r->pool, uriLen + 1);
    ngx_memcpy(pathToCheck, r->uri.data, uriLen);
    pathToCheck[uriLen] = '\0';

    if (conf->nginxModuleIgnorePaths && (conf->nginxModuleIgnorePaths->nelts) > 0) {
        for (ngx_uint_t j = 0; j < conf->nginxModuleIgnorePaths->nelts; j++) {
            const char* data = (const char*)(((ngx_str_t *)(conf->nginxModuleIgnorePaths->elts))[j]).data;
            bool ans = matchIgnorePathRegex(pathToCheck , data);
            if(ans){
                return true;
            }
        }
    }
    return false;
}

static ngx_flag_t ngx_initialize_opentelemetry(ngx_http_request_t *r)
{
    // check to see if we have already been initialized
    if (worker_conf && worker_conf->isInitialized)
    {
        ngx_writeTrace(r->connection->log, __func__, "Opentelemetry SDK already initialized for process with PID: %s", worker_conf->pid);
        return true;
    }

    ngx_http_opentelemetry_loc_conf_t	*conf;
    conf = ngx_http_get_module_loc_conf(r, ngx_http_opentelemetry_module);
    if (conf == NULL)
    {
        ngx_writeError(r->connection->log, __func__, "Module location configuration is NULL");
        return false;
    }

    traceConfig(r, conf);

    if (conf->nginxModuleEnabled)
    {
        OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;
        char            *qs = (char *)malloc(6);
        char            *et = (char *)malloc(6);
        char            *es = (char *)malloc(6);
        char            *sd = (char *)malloc(6);
        ngx_uint_t      i;

        logState = conf->nginxModuleTraceAsError; //Setting Logging Flag

        initDependency();

        struct cNode *cn = ngx_pcalloc(r->pool, sizeof(struct cNode));
        // (cn->cInfo).cName = computeContextName(r, conf);
        struct cNode *rootCN = NULL;
        cn = NULL;


        // Update the apr_pcalloc if we add another parameter to the input array!
        OTEL_SDK_ENV_RECORD* env_config = ngx_pcalloc(r->pool, CONFIG_COUNT * sizeof(OTEL_SDK_ENV_RECORD));
        if(env_config == NULL)
        {
            ngx_writeError(r->connection->log, __func__, "Not Able to allocate memory for the Env Config");
            return false;
        }
        int ix = 0;

        // Otel Exporter Type
        env_config[ix].name = OTEL_SDK_ENV_OTEL_EXPORTER_TYPE;
        env_config[ix].value = (const char*)((conf->nginxModuleOtelSpanExporter).data);
        ++ix;

        // sdk libaray name
        env_config[ix].name = OTEL_SDK_ENV_OTEL_LIBRARY_NAME;
        env_config[ix].value = "Nginx";
        ++ix;

        // Otel Exporter Endpoint
        env_config[ix].name = OTEL_SDK_ENV_OTEL_EXPORTER_ENDPOINT;
        env_config[ix].value = (const char*)(conf->nginxModuleOtelExporterEndpoint).data;
        ++ix;

        // Otel Exporter OTEL headers
        env_config[ix].name = OTEL_SDK_ENV_OTEL_EXPORTER_OTLPHEADERS;
        env_config[ix].value = (const char*)(conf->nginxModuleOtelExporterOtlpHeaders).data;
        ++ix;

        // Otel SSL Enabled
        env_config[ix].name = OTEL_SDK_ENV_OTEL_SSL_ENABLED;
        env_config[ix].value = conf->nginxModuleOtelSslEnabled == 1 ? "1" : "0";
        ++ix;

        // Otel SSL Certificate Path
        env_config[ix].name = OTEL_SDK_ENV_OTEL_SSL_CERTIFICATE_PATH;
        env_config[ix].value = (const char*)(conf->nginxModuleOtelSslCertificatePath).data;
        ++ix;

        // Otel Processor Type
        env_config[ix].name = OTEL_SDK_ENV_OTEL_PROCESSOR_TYPE;
        env_config[ix].value = (const char*)(conf->nginxModuleOtelSpanProcessor).data;
        ++ix;

        // Otel Sampler Type
        env_config[ix].name = OTEL_SDK_ENV_OTEL_SAMPLER_TYPE;
        env_config[ix].value = (const char*)(conf->nginxModuleOtelSampler).data;
        ++ix;

        // Service Namespace
        env_config[ix].name = OTEL_SDK_ENV_SERVICE_NAMESPACE;
        env_config[ix].value = (const char*)(conf->nginxModuleServiceNamespace).data;
        ++ix;

        // Service Name
        env_config[ix].name = OTEL_SDK_ENV_SERVICE_NAME;
        env_config[ix].value = (const char*)(conf->nginxModuleServiceName).data;
        ++ix;

        // Service Instance ID
        env_config[ix].name = OTEL_SDK_ENV_SERVICE_INSTANCE_ID;
        env_config[ix].value = (const char*)(conf->nginxModuleServiceInstanceId).data;
        ++ix;

        // Otel Max Queue Size
        env_config[ix].name = OTEL_SDK_ENV_MAX_QUEUE_SIZE;
        sprintf(qs, "%lu", conf->nginxModuleOtelMaxQueueSize);
        env_config[ix].value = qs;
        ++ix;

        // Otel Scheduled Delay
        env_config[ix].name = OTEL_SDK_ENV_SCHEDULED_DELAY;
        sprintf(sd, "%lu", conf->nginxModuleOtelScheduledDelayMillis);
        env_config[ix].value = sd;
        ++ix;

        // Otel Max Export Batch Size
        env_config[ix].name = OTEL_SDK_ENV_EXPORT_BATCH_SIZE;
        sprintf(es, "%lu", conf->nginxModuleOtelMaxExportBatchSize);
        env_config[ix].value = es;
        ++ix;

        // Otel Export Timeout
        env_config[ix].name = OTEL_SDK_ENV_EXPORT_TIMEOUT;
        sprintf(et, "%lu", conf->nginxModuleOtelExportTimeoutMillis);
        env_config[ix].value = et;
        ++ix;

        // Segment Type
        env_config[ix].name = OTEL_SDK_ENV_SEGMENT_TYPE;
        env_config[ix].value = (const char*)(conf->nginxModuleSegmentType).data;
        ++ix;

        // Segment Parameter
        env_config[ix].name = OTEL_SDK_ENV_SEGMENT_PARAMETER;
        env_config[ix].value = (const char*)(conf->nginxModuleSegmentParameter).data;
        ++ix;

        env_config[ix].name = OTEL_SDK_ENV_OTEL_PROPAGATOR_TYPE;
        env_config[ix].value = (const char*)(conf->nginxModulePropagatorType).data;
        ++ix;

        // !!!
        // Remember to update the ngx_pcalloc call size if we add another parameter to the input array!
        // !!!

        // Adding the webserver context here
        for(int context_i=0; context_i<c_count; context_i++){
            struct cNode *temp_cn  = ngx_pcalloc(r->pool, sizeof(struct cNode));
	    char* name = ngx_pcalloc(r->pool,(contexts[context_i].sNamespace).len + (contexts[context_i].sName).len + (contexts[context_i].sInstanceId).len + 1);
            if(name != NULL){
                strcpy(name, (const char*)(contexts[context_i].sNamespace).data);
                strcat(name, (const char*)(contexts[context_i].sName).data);
                strcat(name, (const char*)(contexts[context_i].sInstanceId).data);
            }
            (temp_cn->cInfo).cName = name;
            (temp_cn->cInfo).sNamespace = (const char*)(contexts[context_i].sNamespace).data;
            (temp_cn->cInfo).sName = (const char*)(contexts[context_i].sName).data;
            (temp_cn->cInfo).sInstanceId = (const char*)(contexts[context_i].sInstanceId).data;
            if(context_i==0)
            {
              cn = temp_cn;
              rootCN = cn;
            }
            else
            {
              cn->next = temp_cn;
              cn = cn->next;
            }
        }
        setRequestResponseHeaders((const char*)(conf->nginxModuleRequestHeaders).data,
           (const char*)(conf->nginxModuleResponseHeaders).data);
        res = opentelemetry_core_init(env_config, ix, rootCN);
        free(qs);
        free(sd);
        free(et);
        free(es);
        if (OTEL_ISSUCCESS(res))
        {
            worker_conf->isInitialized = 1;
            ngx_writeTrace(r->connection->log, __func__, "Initializing Agent Core succceeded for process with PID: %s", worker_conf->pid);
            return true;
        }
        else
        {
           ngx_writeError(r->connection->log, __func__, "Agent Core Init failed, result code is %d", res);
           return false;
        }
    }
    else
    {
        // Agent core is not enabled
        ngx_writeError(r->connection->log, __func__, "Agent Core is not enabled");
        return false;
    }
    return false;
}

static void stopMonitoringRequest(ngx_http_request_t* r,
    OTEL_SDK_HANDLE_REQ request_handle_key)
{
    ngx_http_opentelemetry_loc_conf_t  *ngx_conf = ngx_http_get_module_loc_conf(r, ngx_http_opentelemetry_module);
    if(!ngx_conf->nginxModuleEnabled)
    {
        ngx_writeError(r->connection->log, __func__, "Agent is Disabled");
        return;
    }

    OTEL_SDK_HANDLE_REQ otel_req_handle_key = OTEL_SDK_NO_HANDLE;
    ngx_http_otel_handles_t* ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    if (r->pool == NULL && request_handle_key != NULL)
    {
        otel_req_handle_key = request_handle_key;
    }
    else if (ctx && ctx->otel_req_handle_key)
    {
        otel_req_handle_key = ctx->otel_req_handle_key;
    }
    else
    {
        return;
    }

    ngx_writeTrace(r->connection->log, __func__, "Stopping the Request Monitoring");

    response_payload* res_payload = NULL;
    if (r->pool) {
        res_payload = ngx_pcalloc(r->pool, sizeof(response_payload));
        res_payload->response_headers_count = 0;
        res_payload->otel_attributes_count = 0;
        fillResponsePayload(res_payload, r);
    }
    
    if (r->pool) {
        ngx_pfree(r->pool, ctx);
    }

    OTEL_SDK_STATUS_CODE res;
    char* msg = NULL;

    if (otel_requestHasErrors(r))
    {
        res_payload->status_code = (unsigned int)otel_getErrorCode(r);
        msg = (char*)malloc(STATUS_CODE_BYTE_COUNT * sizeof(char));
        sprintf(msg, "%d", res_payload->status_code);
        res = endRequest(otel_req_handle_key, msg, res_payload);
    }
    else
    {
        res_payload->status_code = r->headers_out.status;
        res = endRequest(otel_req_handle_key, msg, res_payload);
    }

    if (OTEL_ISSUCCESS(res))
    {
        ngx_writeTrace(r->connection->log, __func__, "Request Ends with result code: %d", res);
    }
    else
    {
        ngx_writeError(r->connection->log, __func__, "Request End FAILED with code: %d", res);
    }
    if(msg){
        free(msg);
    }
    
    return;
}

static void startMonitoringRequest(ngx_http_request_t* r){
    // If a not a the main request(sub-request or internal redirect), calls Realip handler and return
    if(r->internal)
    {
        ngx_writeTrace(r->connection->log, __func__, "Not a Main Request(sub-request or internal redirect)");
        return;
    }
    else if (!ngx_initialize_opentelemetry(r))    /* check if Otel Agent Core is initialized */
    {
        ngx_writeError(r->connection->log, __func__, "Opentelemetry Agent Core did not get initialized");
        return;
    }
    else if(check_ignore_paths(r)){
        ngx_writeTrace(r->connection->log, __func__, "Monitoring has been disabled for: %s", r->uri.data);
        return;
    }



    ngx_http_otel_handles_t* ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    if(ctx && ctx->otel_req_handle_key){
        return;
    }

    ngx_writeTrace(r->connection->log, __func__, "Starting Request Monitoring for: %s", r->uri.data);

    // Handle request for static contents (Nginx is used for habdling static contents)

    OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;
    OTEL_SDK_HANDLE_REQ reqHandle = OTEL_SDK_NO_HANDLE;

    const char* wscontext = NULL;

    ngx_http_opentelemetry_loc_conf_t  *ngx_conf = ngx_http_get_module_loc_conf(r, ngx_http_opentelemetry_module);

    if(ngx_conf)
    {
        wscontext = computeContextName(r, ngx_conf);
    }

    if(wscontext)
    {
        ngx_writeTrace(r->connection->log, __func__, "WebServer Context: %s", wscontext);
    }
    else
    {
        ngx_writeTrace(r->connection->log, __func__, "Using Default context ");
    }

    // Fill the Request payload information and start the request monitoring
    request_payload* req_payload = ngx_pcalloc(r->pool, sizeof(request_payload));
    if(req_payload == NULL)
    {
        ngx_writeError(r->connection->log, __func__, "Not able to get memory for request payload");
    }
    fillRequestPayload(req_payload, r);

    res = startRequest(wscontext, req_payload, &reqHandle);

    if (OTEL_ISSUCCESS(res))
    {
        if (ctx == NULL)
        {
            ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_otel_handles_t));
            if (ctx == NULL)
            {
                ngx_writeError(r->connection->log, __func__, "Cannot allocate memory for handles");
                return;
            }
            // Store the Request Handle on the request object
            OTEL_SDK_HANDLE_REQ reqHandleValue = ngx_pcalloc(r->pool, sizeof(OTEL_SDK_HANDLE_REQ));
            if (reqHandleValue)
            {
                reqHandleValue = reqHandle;
                ctx->otel_req_handle_key = reqHandleValue;
                ngx_http_set_ctx(r, ctx, ngx_http_opentelemetry_module);
            }
        }
        ngx_writeTrace(r->connection->log, __func__, "Request Monitoring begins successfully ");
    }
    else if (res == OTEL_STATUS(cfg_channel_uninitialized) || res == OTEL_STATUS(bt_detection_disabled))
    {
        ngx_writeTrace(r->connection->log, __func__, "Request begin detection disabled, result code: %d", res);
    }
    else
    {
        ngx_writeError(r->connection->log, __func__, "Request begin error, result code: %d", res);
    }
}

static ngx_int_t ngx_http_otel_rewrite_handler(ngx_http_request_t *r){
    
    // This will be the first hanndler to be encountered,
    // Here, Init and start the Request Processing by creating Trace, spans etc
    if(!r->internal)
    {
        startMonitoringRequest(r);
    }
    
    otel_startInteraction(r, "ngx_http_rewrite_module");
    ngx_int_t rvalue = h[NGX_HTTP_REWRITE_MODULE_INDEX](r);
    otel_stopInteraction(r, "ngx_http_rewrite_module", OTEL_SDK_NO_HANDLE);

    return rvalue;
}

static ngx_int_t ngx_http_otel_limit_conn_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_limit_conn_module");
    ngx_int_t rvalue = h[NGX_HTTP_LIMIT_CONN_MODULE_INDEX](r);
    otel_stopInteraction(r, "ngx_http_limit_conn_module", OTEL_SDK_NO_HANDLE);

    return rvalue;
}

static ngx_int_t ngx_http_otel_limit_req_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_limit_req_module");
    ngx_int_t rvalue = h[NGX_HTTP_LIMIT_REQ_MODULE_INDEX](r);
    otel_stopInteraction(r, "ngx_http_limit_req_module", OTEL_SDK_NO_HANDLE);

    return rvalue;
}

static ngx_int_t ngx_http_otel_realip_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_realip_module");
    ngx_int_t rvalue = h[NGX_HTTP_REALIP_MODULE_INDEX](r);
    otel_stopInteraction(r, "ngx_http_realip_module", OTEL_SDK_NO_HANDLE);    

    return rvalue;
}

static ngx_int_t ngx_http_otel_auth_request_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_auth_request_module");
    ngx_int_t rvalue = h[NGX_HTTP_LIMIT_AUTH_REQ_MODULE_INDEX](r);
    otel_stopInteraction(r, "ngx_http_auth_request_module", OTEL_SDK_NO_HANDLE);

    return rvalue;
}

static ngx_int_t ngx_http_otel_auth_basic_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_auth_basic_module");
    ngx_int_t rvalue = h[NGX_HTTP_AUTH_BASIC_MODULE_INDEX](r);
    otel_stopInteraction(r, "ngx_http_auth_basic_module", OTEL_SDK_NO_HANDLE);

    return rvalue;
}

static ngx_int_t ngx_http_otel_access_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_access_module");
    ngx_int_t rvalue = h[NGX_HTTP_ACCESS_MODULE_INDEX](r);
    otel_stopInteraction(r, "ngx_http_access_module", OTEL_SDK_NO_HANDLE);

    return rvalue;
}

static ngx_int_t ngx_http_otel_static_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_static_module");
    ngx_int_t rvalue = h[NGX_HTTP_STATIC_MODULE_INDEX](r);
    otel_stopInteraction(r, "ngx_http_static_module", OTEL_SDK_NO_HANDLE);

    return rvalue;
}

static ngx_int_t ngx_http_otel_gzip_static_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_gzip_static_module");
    ngx_int_t rvalue = h[NGX_HTTP_GZIP_STATIC_MODULE_INDEX](r);
    otel_stopInteraction(r, "ngx_http_gzip_static_module", OTEL_SDK_NO_HANDLE);

    return rvalue;
}

static ngx_int_t ngx_http_otel_dav_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_dav_module");
    ngx_int_t rvalue = h[NGX_HTTP_DAV_MODULE_INDEX](r);
    otel_stopInteraction(r, "ngx_http_dav_module", OTEL_SDK_NO_HANDLE);

    return rvalue;
}

static ngx_int_t ngx_http_otel_autoindex_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_autoindex_module");
    ngx_int_t rvalue = h[NGX_HTTP_AUTO_INDEX_MODULE_INDEX](r);
    otel_stopInteraction(r, "ngx_http_autoindex_module", OTEL_SDK_NO_HANDLE);

    return rvalue;
}

/*  autoindex, index and randomindex handlers get called during
    internal redirection. In case of index and randomIndex handlers,
    it has been observed that 'ctx' ptr gets cleaned up from request
    pool and r->internal gets set to 1. But, we need 'ctx' to stop the
    interaction.
    Therefore, as a special handling, we store the ctx pointer and reset
    before stopping the interaction. We avoid starting/stopping
    interaction/request when r->intenal is 1. But in this case,
    when we started the interaction r->internal is 0 but when the
    actual handler calls completes, it internally transforms r->internal
    to 1. Therefore, we need extra handling for r->internal as well.
*/

static ngx_int_t ngx_http_otel_index_handler(ngx_http_request_t *r){
    bool old_internal = r->internal;
    ngx_http_otel_handles_t* old_ctx;

    otel_startInteraction(r, "ngx_http_index_module");
    old_ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    ngx_int_t rvalue = h[NGX_HTTP_INDEX_MODULE_INDEX](r);
    bool new_internal = r->internal;
    if (new_internal == true && new_internal != old_internal) {
        ngx_http_set_ctx(r, old_ctx, ngx_http_opentelemetry_module);
        r->internal = 0;
        otel_stopInteraction(r, "ngx_http_index_module", OTEL_SDK_NO_HANDLE);
        r->internal = 1;
    } else {
        otel_stopInteraction(r, "ngx_http_index_module", OTEL_SDK_NO_HANDLE);
    }

    return rvalue;
}

static ngx_int_t ngx_http_otel_random_index_handler(ngx_http_request_t *r){
    bool old_internal = r->internal;
    ngx_http_otel_handles_t* old_ctx;

    old_ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    otel_startInteraction(r, "ngx_http_random_index_module");
    ngx_int_t rvalue = h[NGX_HTTP_RANDOM_INDEX_MODULE_INDEX](r);
    bool new_internal = r->internal;
    if (new_internal == true && new_internal != old_internal) {
        ngx_http_set_ctx(r, old_ctx, ngx_http_opentelemetry_module);
        r->internal = 0;
        otel_stopInteraction(r, "ngx_http_random_index_module", OTEL_SDK_NO_HANDLE);
        r->internal = 1;

    } else {
        otel_stopInteraction(r, "ngx_http_random_index_module", OTEL_SDK_NO_HANDLE);
    }

    return rvalue;
}

/*  tryfiles handler gets called with try_files tag. In this case
    also, internal redirection happens. But on completion of this
    handler, request pool gets wiped out and since ctx is created
    in request pool, its no longer valid. However, request hanlde
    is not created in request pool. Therefore,we save the request
    handle and pass it along to stop the interaction.  Also,   we
    stop the request using the same request handle.
*/


static ngx_int_t ngx_http_otel_try_files_handler(ngx_http_request_t *r) {
    bool old_internal = r->internal;
    ngx_http_otel_handles_t* old_ctx;

    OTEL_SDK_HANDLE_REQ request_handle = OTEL_SDK_NO_HANDLE;
    old_ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    if (old_ctx && old_ctx->otel_req_handle_key) {
        request_handle = old_ctx->otel_req_handle_key;
    }

    otel_startInteraction(r, "ngx_http_otel_try_files_handler");
    ngx_int_t rvalue = h[NGX_HTTP_TRY_FILES_MODULE_INDEX](r);
    bool new_internal = r->internal;
    if (new_internal == true && new_internal != old_internal) {
        r->internal = 0;
        otel_stopInteraction(r, "ngx_http_otel_try_files_handler", request_handle);
        r->internal = 1;
    } else {
        otel_stopInteraction(r, "ngx_http_otel_try_files_handler", OTEL_SDK_NO_HANDLE);
    }

    if (!r->pool) {
        stopMonitoringRequest(r, request_handle);
    }
    return rvalue;
}

static ngx_int_t ngx_http_otel_mirror_handler(ngx_http_request_t *r) {
    otel_startInteraction(r, "ngx_http_otel_mirror_handler");
    ngx_int_t rvalue = h[NGX_HTTP_MIRROR_MODULE_INDEX](r);
    otel_stopInteraction(r, "ngx_http_otel_mirror_handler", OTEL_SDK_NO_HANDLE);

    return rvalue;
}

static ngx_int_t ngx_http_otel_log_handler(ngx_http_request_t *r){
    //This will be last handler to be be encountered before a request ends and response is finally sent back to client
    // Here, End the main trace, span created by Webserver Agent and the collected data will be passed to the backend
    // It will work as ngx_http_opentelemetry_log_transaction_end
    stopMonitoringRequest(r, OTEL_SDK_NO_HANDLE);

    ngx_int_t rvalue = h[NGX_HTTP_LOG_MODULE_INDEX](r);

    return rvalue;
}

static ngx_int_t isOTelMonitored(const char* str){
    unsigned int i = 0;
    for(i=0; i<NGX_HTTP_MAX_HANDLE_COUNT; i++){
        if(strcmp(str, otel_monitored_modules[i].name) == 0)
            return i;
        }
    return -1;
}

static char* computeContextName(ngx_http_request_t *r, ngx_http_opentelemetry_loc_conf_t* conf){
    char* name = ngx_pcalloc(r->pool,(conf->nginxModuleServiceNamespace).len + (conf->nginxModuleServiceName).len + (conf->nginxModuleServiceInstanceId).len + 1);

    if(name != NULL){
        strcpy(name, (const char*)(conf->nginxModuleServiceNamespace).data);
        strcat(name, (const char*)(conf->nginxModuleServiceName).data);
        strcat(name, (const char*)(conf->nginxModuleServiceInstanceId).data);
    }
    return name;
}

static void traceConfig(ngx_http_request_t *r, ngx_http_opentelemetry_loc_conf_t* conf){
    ngx_writeTrace(r->connection->log, __func__, " Config { :"
                                                      "(Enabled=\"%ld\")"
                                                      "(OtelExporterEndpoint=\"%s\")"
                                                      "(OtelExporterOtlpHeader=\"%s\")"
                                                      "(OtelSslEnabled=\"%ld\")"
                                                      "(OtelSslCertificatePath=\"%s\")"
                                                      "(OtelSpanExporter=\"%s\")"
                                                      "(OtelSpanProcessor=\"%s\")"
                                                      "(OtelSampler=\"%s\")"
                                                      "(ServiceNamespace=\"%s\")"
                                                      "(ServiceName=\"%s\")"
                                                      "(ServiceInstanceId=\"%s\")"
                                                      "(OtelMaxQueueSize=\"%lu\")"
                                                      "(OtelScheduledDelayMillis=\"%lu\")"
                                                      "(OtelExportTimeoutMillis=\"%lu\")"
                                                      "(OtelMaxExportBatchSize=\"%lu\")"
                                                      "(ResolveBackends=\"%ld\")"
                                                      "(TraceAsError=\"%ld\")"
                                                      "(ReportAllInstrumentedModules=\"%ld\")"
                                                      "(MaskCookie=\"%ld\")"
                                                      "(MaskSmUser=\"%ld\")"
                                                      "(SegmentType=\"%s\")"
                                                      "(SegmentParameter=\"%s\")"
                                                      " }",
                                                      conf->nginxModuleEnabled,
                                                      (conf->nginxModuleOtelExporterEndpoint).data,
                                                      (conf->nginxModuleOtelExporterOtlpHeaders).data,
                                                      conf->nginxModuleOtelSslEnabled,
                                                      (conf->nginxModuleOtelSslCertificatePath).data,
                                                      (conf->nginxModuleOtelSpanExporter).data,
                                                      (conf->nginxModuleOtelSpanProcessor).data,
                                                      (conf->nginxModuleOtelSampler).data,
                                                      (conf->nginxModuleServiceNamespace).data,
                                                      (conf->nginxModuleServiceName).data,
                                                      (conf->nginxModuleServiceInstanceId).data,
                                                      conf->nginxModuleOtelMaxQueueSize,
                                                      conf->nginxModuleOtelScheduledDelayMillis,
                                                      conf->nginxModuleOtelExportTimeoutMillis,
                                                      conf->nginxModuleOtelMaxExportBatchSize,
                                                      conf->nginxModuleResolveBackends,
                                                      conf->nginxModuleTraceAsError,
                                                      conf->nginxModuleReportAllInstrumentedModules,
                                                      conf->nginxModuleMaskCookie,
                                                      conf->nginxModuleMaskSmUser,
                                                      (conf->nginxModuleSegmentType).data,
                                                      (conf->nginxModuleSegmentParameter).data);
}

static void removeUnwantedHeader(ngx_http_request_t* r)
{
  ngx_list_part_t  *part;
  ngx_table_elt_t  *header;
  ngx_table_elt_t            *h;
  ngx_http_header_t          *hh;
  ngx_http_core_main_conf_t  *cmcf;
  ngx_uint_t       nelts;

  part = &r->headers_in.headers.part;
  header = (ngx_table_elt_t*)part->elts;
  nelts = part->nelts;

  for(ngx_uint_t j = 0; j<nelts; j++){
    h = &header[j];
    if(strcmp("singularityheader", h->key.data)==0){
      if (h->value.len == 0) {
        break;
      }
      if(h->value.data)
        ngx_pfree(r->pool, h->value.data);

      char str[] = "";
      h->hash = ngx_hash_key(h->key.data, h->key.len);

      h->value.len = 0;
      h->value.data = ngx_pcalloc(r->pool, sizeof(char)*((h->value.len)+1));
      strcpy(h->value.data, str);
      h->lowcase_key = h->key.data;

      cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
      hh = ngx_hash_find(&cmcf->headers_in_hash, h->hash,h->lowcase_key, h->key.len);
      if (hh && hh->handler(r, h, hh->offset) != NGX_OK) {
       return;
      }

      break;
    }
  }
}


static void fillRequestPayload(request_payload* req_payload, ngx_http_request_t* r){
    ngx_list_part_t  *part;
    ngx_table_elt_t  *header;
    ngx_uint_t       nelts;
    ngx_table_elt_t  *h;

    // creating a temporary uri for uri parsing 
    // (r->uri).data has an extra component "HTTP/1.1 connection" so to obtain the uri it
    // has to trimmed. This is done by putting a '/0' after the uri length
    // WEBSRV-558
    char *temp_uri = ngx_pcalloc(r->pool, (strlen((r->uri).data))+1);
    strcpy(temp_uri,(const char*)(r->uri).data);
    temp_uri[(r->uri).len]='\0';
    req_payload->uri = temp_uri;

    ngx_http_core_srv_conf_t* cscf = (ngx_http_core_srv_conf_t*)ngx_http_get_module_srv_conf(r, ngx_http_core_module);
    req_payload->server_name = (const char*)(cscf->server_name).data;

    #if (NGX_HTTP_SSL)

      if(r->connection->ssl)
      {
        req_payload->scheme = "https";
      }
      else
      {
        req_payload->scheme = "http";
      }

    #else

      req_payload->scheme = "http";

    #endif

    // TODO - use strncpy function to just create memory of size (r->http_protocol.len)
    char *temp_http_protocol = ngx_pcalloc(r->pool, (strlen((r->http_protocol).data))+1);
    strcpy(temp_http_protocol,(const char*)(r->http_protocol).data);
    temp_http_protocol[(r->http_protocol).len]='\0';
    req_payload->protocol = temp_http_protocol;

    char *temp_request_method = ngx_pcalloc(r->pool, (strlen((r->method_name).data))+1);
    strcpy(temp_request_method,(const char*)(r->method_name).data);
    temp_request_method[(r->method_name).len]='\0';
    req_payload->request_method = temp_request_method;

    char *temp_user_agent = ngx_pcalloc(r->pool, r->headers_in.user_agent->value.len +1);
    strcpy(temp_user_agent,(const char*)(r->headers_in.user_agent->value.data));
    temp_user_agent[r->headers_in.user_agent->value.len]='\0';
    req_payload->user_agent = temp_user_agent;

    ngx_uint_t remote_port = 0;
    if (r->connection != NULL) {
        struct sockaddr *temp_sockaddress = r->connection->sockaddr;
        if(temp_sockaddress->sa_family == 2) {
                struct sockaddr_in *temp_soc = (struct sockaddr_in *) temp_sockaddress;
                remote_port = ntohs(temp_soc->sin_port);
        }
        else if(temp_sockaddress->sa_family == 10 || temp_sockaddress->sa_family == 23){
                struct sockaddr_in6 *temp_soc6 = (struct sockaddr_in6 *) temp_sockaddress;
                remote_port = ntohs(temp_soc6->sin6_port);
        }
    }
    req_payload->peer_port = remote_port;
    // flavor has to be scraped from protocol in future
    req_payload->flavor = temp_http_protocol;

    char *temp_hostname = ngx_pcalloc(r->pool, (strlen(hostname.data))+1);
    strcpy(temp_hostname,(const char*)hostname.data);
    temp_hostname[hostname.len]='\0';
    req_payload->hostname = temp_hostname;

    req_payload->http_post_param = ngx_pcalloc(r->pool, sizeof(u_char*));
    req_payload->http_get_param = ngx_pcalloc(r->pool, sizeof(u_char*));

    if(strstr(req_payload->request_method, "GET") !=NULL){
        req_payload->http_post_param = "No param";
        if((r->args).len){
            req_payload->http_get_param = (const char*)(r->args).data;
        }else{
            req_payload->http_get_param = "No param";
        }
    }else if(strstr(req_payload->request_method, "POST") != NULL){
        req_payload->http_get_param = "No param";
        if((r->args).len){
            req_payload->http_post_param = (const char*)(r->args).data;
        }else{
            req_payload->http_post_param = "No param";
        }
    }

    req_payload->client_ip = (const char*)(r->connection->addr_text).data;
    char *temp_client_ip = ngx_pcalloc(r->pool, (strlen((r->connection->addr_text).data))+1);
    strcpy(temp_client_ip,(const char*)(r->connection->addr_text).data);
    temp_client_ip[(r->connection->addr_text).len]='\0';
    req_payload->client_ip = temp_client_ip;

    ngx_http_opentelemetry_loc_conf_t *conf =
      ngx_http_get_module_loc_conf(r, ngx_http_opentelemetry_module);

    req_payload->operation_name = (const char*)conf->nginxModuleOperationName.data;

    part = &r->headers_in.headers.part;
    header = (ngx_table_elt_t*)part->elts;
    nelts = part->nelts;

    req_payload->propagation_headers = ngx_pcalloc(r->pool, nelts * sizeof(http_headers));
    req_payload->request_headers = ngx_pcalloc(r->pool, nelts * sizeof(http_headers));
    int request_headers_idx = 0;
    int propagation_headers_idx = 0;
    for (ngx_uint_t j = 0; j < nelts; j++) {
        h = &header[j];
        req_payload->request_headers[request_headers_idx].name = (char*)(h->key).data;
        req_payload->request_headers[request_headers_idx].value = (char*)(h->value).data;
        if (req_payload->request_headers[request_headers_idx].value == NULL) {
            req_payload->request_headers[request_headers_idx].value = "";
        }
        request_headers_idx++;
    }

    for (ngx_uint_t j = 0 ;; j++) {
        if (j >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            header = (ngx_table_elt_t*)part->elts;
            j = 0;
        }
        h = &header[j];
        for (int i = 0; i < headers_len && conf->nginxModuleTrustIncomingSpans ; i++) {
            
            if (strcmp(h->key.data, httpHeaders[i]) == 0) {
                req_payload->propagation_headers[propagation_headers_idx].name = httpHeaders[i];
                req_payload->propagation_headers[propagation_headers_idx].value = (const char*)(h->value).data;
                if (req_payload->propagation_headers[propagation_headers_idx].value == NULL) {
                    req_payload->propagation_headers[propagation_headers_idx].value = "";
                }
                propagation_headers_idx++;
                break;
            }
        }
    }
    req_payload->propagation_count = propagation_headers_idx;
    req_payload->request_headers_count = request_headers_idx;

}

static void fillResponsePayload(response_payload* res_payload, ngx_http_request_t* r)
{
    if (!r->pool) {
        return;
    }

    ngx_list_part_t  *part;
    ngx_table_elt_t  *header;
    ngx_uint_t       nelts;
    ngx_table_elt_t  *h;

    part = &r->headers_out.headers.part;
    header = (ngx_table_elt_t*)part->elts;
    nelts = part->nelts;

    res_payload->response_headers = ngx_pcalloc(r->pool, nelts * sizeof(http_headers));
    ngx_uint_t headers_count = 0;

    ngx_http_opentelemetry_loc_conf_t *conf = ngx_http_get_module_loc_conf(r, ngx_http_opentelemetry_module);
    
    if (conf->nginxModuleAttributes && (conf->nginxModuleAttributes->nelts) > 0) {

        res_payload->otel_attributes = ngx_pcalloc(r->pool, ((conf->nginxModuleAttributes->nelts + 1)/3) * sizeof(http_headers));
        ngx_uint_t otel_attributes_idx=0;

        for (ngx_uint_t j = 0, isKey = 1, isValue = 0; j < conf->nginxModuleAttributes->nelts; j++) {

            ngx_str_t var_name = (((ngx_str_t *)(conf->nginxModuleAttributes->elts))[j]);
            ngx_uint_t           key; // The variable's hashed key.
            ngx_http_variable_value_t  *value; // Pointer to the value object.

            if(var_name.data[0] == NGINX_VARIABLE_IDENTIFIER){
                // Get the hashed key.
                ngx_str_t new_var_name = var_name;
                new_var_name.data++;
                new_var_name.len--;
                key = ngx_hash_key(new_var_name.data, new_var_name.len);

                // Get the variable.
                value = ngx_http_get_variable(r, &new_var_name, key);
                if (!(value == NULL || value->not_found)) {
                    var_name.data = value->data;
                    var_name.len = value->len;
                } 
            }
            
            char* data = ngx_pcalloc(r->pool, var_name.len +1);
            ngx_memcpy(data, (const char*)(var_name.data) , var_name.len);
            data[var_name.len] = '\0';

            if(strcmp(data, ",") == 0){
                otel_attributes_idx++;
                continue;
            }
            else if(isKey){
                res_payload->otel_attributes[otel_attributes_idx].name = data; 
            }
            else{
                res_payload->otel_attributes[otel_attributes_idx].value = data;
            } 
            isKey=!isKey;
            isValue=!isValue;
        }
        res_payload->otel_attributes_count = otel_attributes_idx+1;
    }

    for (ngx_uint_t j = 0; j < nelts; j++) {
        h = &header[j];

        if (headers_count < nelts) {
            res_payload->response_headers[headers_count].name = (const char*)(h->key).data;
            res_payload->response_headers[headers_count].value = (const char*)(h->value).data;
            if (res_payload->response_headers[headers_count].value == NULL) {
                res_payload->response_headers[headers_count].value = "";
            }
            headers_count++;
        }
    }
    res_payload->response_headers_count = headers_count;
}
