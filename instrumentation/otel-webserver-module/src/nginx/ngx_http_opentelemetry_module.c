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

#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "ngx_http_opentelemetry_module.h"
#include "ngx_http_opentelemetry_log.h"

ngx_http_opentelemetry_worker_conf_t *worker_conf;
static contextNode contexts[5];
static unsigned int c_count = 0;
static unsigned int isGlobalContextSet = 0;

/*
List of modules being monitored
*/
otel_ngx_module otel_monitored_modules[] = {
    {
        "ngx_http_realip_module",
        0,
        {NGX_HTTP_POST_READ_PHASE, NGX_HTTP_PREACCESS_PHASE},
        ngx_http_otel_realip_handler,
        0,
        2
    },
    {
        "ngx_http_rewrite_module",
        0,
        {NGX_HTTP_SERVER_REWRITE_PHASE, NGX_HTTP_REWRITE_PHASE},
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
    }
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

    ngx_null_command	/* command termination */
};

/* The module context. */
static ngx_http_module_t ngx_http_opentelemetry_module_ctx = {
    NULL,						/* preconfiguration */
    ngx_http_opentelemetry_init,	                        /* postconfiguration */

    NULL,	                                        /* create main configuration */
    NULL,	                                        /* init main configuration */

    NULL,	                                        /* create server configuration */
    NULL,	                                        /* merge server configuration */

    ngx_http_opentelemetry_create_loc_conf,	        /* create location configuration */
    ngx_http_opentelemetry_merge_loc_conf		        /* merge location configuration */
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

    return conf;
}

static char* ngx_http_opentelemetry_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_opentelemetry_loc_conf_t *prev = parent;
    ngx_http_opentelemetry_loc_conf_t *conf = child;
    ngx_otel_set_global_context(prev);

    ngx_conf_merge_value(conf->nginxModuleEnabled, prev->nginxModuleEnabled, 1);
    ngx_conf_merge_value(conf->nginxModuleReportAllInstrumentedModules, prev->nginxModuleReportAllInstrumentedModules, 0);
    ngx_conf_merge_value(conf->nginxModuleResolveBackends, prev->nginxModuleResolveBackends, 1);
    ngx_conf_merge_value(conf->nginxModuleTraceAsError, prev->nginxModuleTraceAsError, 0);
    ngx_conf_merge_value(conf->nginxModuleMaskCookie, prev->nginxModuleMaskCookie, 0);
    ngx_conf_merge_value(conf->nginxModuleMaskSmUser, prev->nginxModuleMaskSmUser, 0);

    ngx_conf_merge_str_value(conf->nginxModuleOtelSpanExporter, prev->nginxModuleOtelSpanExporter, "");
    ngx_conf_merge_str_value(conf->nginxModuleOtelExporterEndpoint, prev->nginxModuleOtelExporterEndpoint, "");
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
static ngx_int_t ngx_http_opentelemetry_init(ngx_conf_t *cf)
{
    ngx_http_core_main_conf_t    *cmcf;
    ngx_uint_t                   m, cp, ap, pap, srp, prp, rp, lp;
    ngx_http_phases              ph;
    ngx_uint_t                   phase_index;
    ngx_int_t                    res;

    ngx_writeError(cf->cycle->log, __func__, "Starting Opentelemetry Modlue init");

    cp = ap = pap = srp = prp = rp = lp = 0;
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

                        case NGX_HTTP_FIND_CONFIG_PHASE:
                        case NGX_HTTP_POST_REWRITE_PHASE:
                        case NGX_HTTP_POST_ACCESS_PHASE:
                        case NGX_HTTP_PRECONTENT_PHASE:
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

    ngx_writeError(cf->cycle->log, __func__, "Opentelemetry Modlue init completed !");

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

/*
    Begin a new interaction
*/
static APPD_SDK_STATUS_CODE otel_startInteraction(ngx_http_request_t* r, const char* module_name){
    APPD_SDK_STATUS_CODE res = APPD_SUCCESS;
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
        APPD_SDK_ENV_RECORD* propagationHeaders = ngx_pcalloc(r->pool, 5 * sizeof(APPD_SDK_ENV_RECORD));
        if (propagationHeaders == NULL)
        {
            ngx_writeError(r->connection->log, __func__, "Failed to allocate memory for propagation headers");
            return APPD_STATUS(fail);
        }
        ngx_writeTrace(r->connection->log, __func__, "Starting a new module interaction for: %s", module_name); 
        int ix = 0;
        res = startModuleInteraction(ctx->otel_req_handle_key, module_name, "", resolveBackends, propagationHeaders, &ix);

        if (APPD_ISSUCCESS(res))
        {
            otel_payload_decorator(r, propagationHeaders, ix);
            ngx_writeTrace(r->connection->log, __func__, "Interaction begin successful");
        }
        else
        {
            ngx_writeError(r->connection->log, __func__, "Error: Interaction begin result code: %d", res);
        }
    }
    return res;
}

static void otel_payload_decorator(ngx_http_request_t* r, APPD_SDK_ENV_RECORD* propagationHeaders, int count)
{
   ngx_table_elt_t            *h;
   ngx_http_header_t          *hh;
   ngx_http_core_main_conf_t  *cmcf;
/*
   for(int i=0; i<count; i++){
       h = ngx_list_push(&r->headers_in.headers);
       if(h == NULL){
           return;
       }
       h->key.len = strlen(propagationHeaders[i].name);
       h->key.data = propagationHeaders[i].name;

       ngx_writeTrace(r->connection->log, __func__, "Key : %s", propagationHeaders[i].name);

       h->hash = ngx_hash_key(h->key.data, h->key.len);

       h->value.len = strlen(propagationHeaders[i].value);
       h->value.data = propagationHeaders[i].value;
       h->lowcase_key = h->key.data;

       cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
       hh = ngx_hash_find(&cmcf->headers_in_hash, h->hash,h->lowcase_key, h->key.len);
       if (hh && hh->handler(r, h, hh->offset) != NGX_OK) {
           return;
       }

       ngx_writeTrace(r->connection->log, __func__, "Value : %s", propagationHeaders[i].value);
   }*/
   
   ngx_http_otel_handles_t* ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
   ctx->propagationHeaders = propagationHeaders;
   ctx->pheaderCount = count;
}

/*
    Stopping an Interaction
*/
static void otel_stopInteraction(ngx_http_request_t* r, const char* module_name)
{
    if(!r || r->internal)
    {
        return;
    }

    ngx_http_otel_handles_t* ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    if(ctx && ctx->otel_req_handle_key)
    {
        // TODO: Work on backend naming and type
        char* backendName = ngx_pcalloc(r->pool, 6);
        *backendName = '\0';
        const char* backendType = "HTTP";
        unsigned int errCode=0;
        char* code = ngx_pcalloc(r->pool, 6);

        const char* status = "HTTP Status Code: ";
        char* msg = ngx_pcalloc(r->pool, strlen(status) + 6);
        *msg = '\0';
        if(otel_requestHasErrors(r))
        {
            errCode=(unsigned int)otel_getErrorCode(r);
            sprintf(code, "%d", errCode);
            strcpy(msg, status);
            strcat(msg, code);
        }
        ngx_writeTrace(r->connection->log, __func__, "Stopping the Interaction for: %s", module_name);
        APPD_SDK_STATUS_CODE res = stopModuleInteraction(ctx->otel_req_handle_key, backendName, backendType, errCode, msg);
        if (APPD_ISFAIL(res))
        {
            ngx_writeError(r->connection->log, __func__, "Error: Stop Interaction failed, result code: %d", res);
        }
    }
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
        APPD_SDK_STATUS_CODE res = APPD_SUCCESS;
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
        APPD_SDK_ENV_RECORD* env_config = ngx_pcalloc(r->pool, 16 * sizeof(APPD_SDK_ENV_RECORD));
        if(env_config == NULL)
        {
            ngx_writeError(r->connection->log, __func__, "Not Able to allocate memory for the Env Config");
            return false;
        }
        int ix = 0;

        // Otel Exporter Type
        env_config[ix].name = APPD_SDK_ENV_OTEL_EXPORTER_TYPE;
        env_config[ix].value = (const char*)((conf->nginxModuleOtelSpanExporter).data);
        ++ix;

        // sdk libaray name
        env_config[ix].name = APPD_SDK_ENV_OTEL_LIBRARY_NAME;
        env_config[ix].value = "Nginx";
        ++ix;

        // Otel Exporter Endpoint
        env_config[ix].name = APPD_SDK_ENV_OTEL_EXPORTER_ENDPOINT;
        env_config[ix].value = (const char*)(conf->nginxModuleOtelExporterEndpoint).data;
        ++ix;

        // Otel SSL Enabled
        env_config[ix].name = APPD_SDK_ENV_OTEL_SSL_ENABLED;
        env_config[ix].value = conf->nginxModuleOtelSslEnabled == 1 ? "1" : "0";
        ++ix;

        // Otel SSL Certificate Path
        env_config[ix].name = APPD_SDK_ENV_OTEL_SSL_CERTIFICATE_PATH;
        env_config[ix].value = (const char*)(conf->nginxModuleOtelSslCertificatePath).data;
        ++ix;

        // Otel Processor Type
        env_config[ix].name = APPD_SDK_ENV_OTEL_PROCESSOR_TYPE;
        env_config[ix].value = (const char*)(conf->nginxModuleOtelSpanProcessor).data;
        ++ix;

        // Otel Sampler Type
        env_config[ix].name = APPD_SDK_ENV_OTEL_SAMPLER_TYPE;
        env_config[ix].value = (const char*)(conf->nginxModuleOtelSampler).data;
        ++ix;

        // Service Namespace
        env_config[ix].name = APPD_SDK_ENV_SERVICE_NAMESPACE;
        env_config[ix].value = (const char*)(conf->nginxModuleServiceNamespace).data;
        ++ix;

        // Service Name
        env_config[ix].name = APPD_SDK_ENV_SERVICE_NAME;
        env_config[ix].value = (const char*)(conf->nginxModuleServiceName).data;
        ++ix;

        // Service Instance ID
        env_config[ix].name = APPD_SDK_ENV_SERVICE_INSTANCE_ID;
        env_config[ix].value = (const char*)(conf->nginxModuleServiceInstanceId).data;
        ++ix;

        // Otel Max Queue Size
        env_config[ix].name = APPD_SDK_ENV_MAX_QUEUE_SIZE;
        sprintf(qs, "%d", conf->nginxModuleOtelMaxQueueSize);
        env_config[ix].value = qs;
        ++ix;

        // Otel Scheduled Delay
        env_config[ix].name = APPD_SDK_ENV_SCHEDULED_DELAY;
        sprintf(sd, "%d", conf->nginxModuleOtelScheduledDelayMillis);
        env_config[ix].value = sd;
        ++ix;

        // Otel Max Export Batch Size
        env_config[ix].name = APPD_SDK_ENV_EXPORT_BATCH_SIZE;
        sprintf(es, "%d", conf->nginxModuleOtelMaxExportBatchSize);
        env_config[ix].value = es;
        ++ix;

        // Otel Export Timeout
        env_config[ix].name = APPD_SDK_ENV_EXPORT_TIMEOUT;
        sprintf(et, "%d", conf->nginxModuleOtelExportTimeoutMillis);
        env_config[ix].value = et;
        ++ix;

        // Segment Type
        env_config[ix].name = APPD_SDK_ENV_SEGMENT_TYPE;
        env_config[ix].value = (const char*)(conf->nginxModuleSegmentType).data;
        ++ix;

        // Segment Parameter
        env_config[ix].name = APPD_SDK_ENV_SEGMENT_PARAMETER;
        env_config[ix].value = (const char*)(conf->nginxModuleSegmentParameter).data;
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

        res = opentelemetry_core_init(env_config, ix, rootCN);
        free(qs);
        free(sd);
        free(et);
        free(es);
        if (APPD_ISSUCCESS(res))
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

static void stopMonitoringRequest(ngx_http_request_t* r)
{
    if(r->internal)
    {
        return;
    }

    ngx_http_opentelemetry_loc_conf_t  *ngx_conf = ngx_http_get_module_loc_conf(r, ngx_http_opentelemetry_module);
    if(!ngx_conf->nginxModuleEnabled)
    {
        ngx_writeError(r->connection->log, __func__, "Agent is Disabled");
    }

    ngx_http_otel_handles_t* ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    if(!ctx || !ctx->otel_req_handle_key)
    {
        return;
    }
    APPD_SDK_HANDLE_REQ reqHandle = (APPD_SDK_HANDLE_REQ)ctx->otel_req_handle_key;
    ngx_pfree(r->pool, ctx);

    ngx_writeTrace(r->connection->log, __func__, "Stopping the Request Monitoring");

    APPD_SDK_STATUS_CODE res;
    unsigned int errCode=0;
    char* msg = NULL;

    if (otel_requestHasErrors(r))
    {
        errCode=(unsigned int)otel_getErrorCode(r);
        msg = (char*)malloc(6);
        sprintf(msg, "%d", errCode);
        res = endRequest(reqHandle, msg);
    }
    else
    {
        res = endRequest(reqHandle, msg);
    }

    if (APPD_ISSUCCESS(res))
    {
        ngx_writeError(r->connection->log, __func__, "Request Ends with result code: %d", res);
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
    else if (!ngx_initialize_opentelemetry(r))    /* check if Appd Agent Core is initialized */
    {
        ngx_writeError(r->connection->log, __func__, "Opentelemetry Agent Core did not get initialized");
        return;
    }

    ngx_http_otel_handles_t* ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_opentelemetry_module);
    if(ctx && ctx->otel_req_handle_key){
        return;
    }

    ngx_writeError(r->connection->log, __func__, "Starting Request Monitoring for: %s", r->uri.data);

    // Handle request for static contents (Nginx is used for habdling static contents)

    APPD_SDK_STATUS_CODE res = APPD_SUCCESS;
    APPD_SDK_HANDLE_REQ reqHandle = APPD_SDK_NO_HANDLE;

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
    int header_count = 0;
    fillRequestPayload(req_payload, r, &header_count);

    res = startRequest(wscontext, req_payload, &reqHandle, header_count);

    if (APPD_ISSUCCESS(res))
    {
        if (ctx == NULL)
        {
            ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_otel_handles_t));
            if (ctx == NULL)
            {
                ngx_writeError(r->connection->log, __func__, "Cannot allocate memory for handles");
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Cannot allocate memory for handles");
                return;
            }
            // Store the Request Handle on the request object
            APPD_SDK_HANDLE_REQ reqHandleValue = ngx_pcalloc(r->pool, sizeof(APPD_SDK_HANDLE_REQ));
            if (reqHandleValue)
            {
                reqHandleValue = reqHandle;
                ctx->otel_req_handle_key = reqHandleValue;
                ngx_http_set_ctx(r, ctx, ngx_http_opentelemetry_module);
            }
        }
        ngx_writeTrace(r->connection->log, __func__, "Request Monitoring begins successfully ");
    }
    else if (res == APPD_STATUS(cfg_channel_uninitialized) || res == APPD_STATUS(bt_detection_disabled))
    {
        ngx_writeTrace(r->connection->log, __func__, "Request begin detection disabled, result code: %d", res);
    }
    else
    {
        ngx_writeError(r->connection->log, __func__, "Request begin error, result code: %d", res);
    }
}

static ngx_int_t ngx_http_otel_rewrite_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_rewrite_module");
    ngx_int_t rvalue = h[0](r);
    otel_stopInteraction(r, "ngx_http_rewrite_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_limit_conn_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_limit_conn_module");
    ngx_int_t rvalue = h[1](r);
    otel_stopInteraction(r, "ngx_http_limit_conn_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_limit_req_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_limit_req_module");
    ngx_int_t rvalue = h[2](r);
    otel_stopInteraction(r, "ngx_http_limit_req_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_realip_handler(ngx_http_request_t *r){

    // This will be the first hanndler to be encountered,
    // Here, Init and start the Request Processing by creating Trace, spans etc
    if(!r->internal)
    {
        startMonitoringRequest(r);
    }

    otel_startInteraction(r, "ngx_http_realip_module");
    ngx_int_t rvalue = h[3](r);
    otel_stopInteraction(r, "ngx_http_realip_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_auth_request_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_auth_request_module");
    ngx_int_t rvalue = h[4](r);
    otel_stopInteraction(r, "ngx_http_auth_request_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_auth_basic_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_auth_basic_module");
    ngx_int_t rvalue = h[5](r);
    otel_stopInteraction(r, "ngx_http_auth_basic_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_access_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_access_module");
    ngx_int_t rvalue = h[6](r);
    otel_stopInteraction(r, "ngx_http_access_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_static_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_static_module");
    ngx_int_t rvalue = h[7](r);
    otel_stopInteraction(r, "ngx_http_static_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_gzip_static_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_gzip_static_module");
    ngx_int_t rvalue = h[8](r);
    otel_stopInteraction(r, "ngx_http_gzip_static_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_dav_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_dav_module");
    ngx_int_t rvalue = h[9](r);
    otel_stopInteraction(r, "ngx_http_dav_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_autoindex_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_autoindex_module");
    ngx_int_t rvalue = h[10](r);
    otel_stopInteraction(r, "ngx_http_autoindex_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_index_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_index_module");
    ngx_int_t rvalue = h[11](r);
    otel_stopInteraction(r, "ngx_http_index_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_random_index_handler(ngx_http_request_t *r){
    otel_startInteraction(r, "ngx_http_random_index_module");
    ngx_int_t rvalue = h[12](r);
    otel_stopInteraction(r, "ngx_http_random_index_module");

    return rvalue;
}

static ngx_int_t ngx_http_otel_log_handler(ngx_http_request_t *r){
    //This will be last handler to be be encountered before a request ends and response is finally sent back to client
    // Here, End the main trace, span created by Webserver Agent and the collected data will be passed to the backend
    // It will work as ngx_http_opentelemetry_log_transaction_end
    if(!r->internal)
    {
        stopMonitoringRequest(r);
    }

    ngx_int_t rvalue = h[13](r);

    return rvalue;
}

static ngx_int_t isOTelMonitored(const char* str){
    unsigned int i = 0;
    for(i=0; i<14; i++){
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
                                                      "(OtelMaxQueueSize=\"%d\")"
                                                      "(OtelScheduledDelayMillis=\"%d\")"
                                                      "(OtelExportTimeoutMillis=\"%d\")"
                                                      "(OtelMaxExportBatchSize=\"%d\")"
                                                      "(ResolveBackends=\"%d\")"
                                                      "(TraceAsError=\"%d\")"
                                                      "(ReportAllInstrumentedModules=\"%d\")"
                                                      "(MaskCookie=\"%d\")"
                                                      "(MaskSmUser=\"%d\")"
                                                      "(SegmentType=\"%s\")"
                                                      "(SegmentParameter=\"%s\")"
                                                      " }",
                                                      conf->nginxModuleEnabled,
                                                      (conf->nginxModuleOtelExporterEndpoint).data,
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

static void fillRequestPayload(request_payload* req_payload, ngx_http_request_t* r, int* count){
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

    req_payload->protocol = (const char*)(r->http_protocol).data;
    req_payload->request_method = (const char*)(r->method_name).data;

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

   part = &r->headers_in.headers.part;
   header = (ngx_table_elt_t*)part->elts;
   nelts = part->nelts;

   req_payload->headers = ngx_pcalloc(r->pool, 3 * sizeof(http_headers));
   for(int i=0; i<3; i++){
       char* c = httpHeaders[i];
       req_payload->headers[*count].name = c;
       for(ngx_uint_t j = 0; j<nelts; j++){
           h = &header[j];
           if(strcmp(httpHeaders[i], h->key.data)==0){
               req_payload->headers[*count].value = (const char*)(h->value).data;
           }
       }
       if(!req_payload->headers[*count].value){
           req_payload->headers[*count].value = "No Param";
       }
       ++(*count);
   }
}
