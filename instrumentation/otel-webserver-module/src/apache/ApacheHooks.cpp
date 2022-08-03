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
// file header
#include "ApacheHooks.h"
// system headers
#include <sstream>
#include <unordered_set>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
// apache headers
#include <apr_strings.h>
#include <httpd.h>
#include <http_config.h> // ap_get_module_config and others
#include <http_log.h> // ap_log_error and others
#include <http_protocol.h> // ap_hook_get_pre_read_request and others
#include <http_request.h> // ap_hook_get_create_request and others
// module headers
#include <api/WSAgent.h>
#include <api/Payload.h>
#include "ApacheTracing.h"
#include "ApacheConfig.h"

#include "sdkwrapper/SdkConstants.h"

std::string ApacheHooks::m_aggregatorCommDir = "";
bool ApacheHooks::m_reportAllStages = false;
const std::initializer_list<const char*> ApacheHooks::httpHeaders = {
    "baggage",
    "traceparent",
    "tracestate"
};
const char* ApacheHooks::APPD_CONFIG_KEY = "appd_configuration";
const char* ApacheHooks::APPD_CORRELATION_HEADER_KEY = "singularityheader";
const char* ApacheHooks::APPD_INTERACTION_HANDLE_KEY = "appd_interaction_handle_key";
const char* ApacheHooks::APPD_REQ_HANDLE_KEY = "appd_req_handle_key";
const char* APPD_OUTPUT_FILTER_NAME = "APPD_EUM_AUTOINJECT";

appd::core::WSAgent wsAgent; // global variable for interface between Hooks and Core Logic

using namespace appd::core::sdkwrapper;

void ApacheHooks::registerHooks(apr_pool_t *p)
{
    // Place a hook that executes when a child process is spawned
    // (commonly used for initializing modules after the server has forked)
    ap_hook_child_init(appd_child_init, NULL, NULL, APR_HOOK_MIDDLE);

    //------------------------------------------------------------------------------
    // The -2 and +2 below is a trick to ensure that our hooks are before and after
    // the majority of the modules within a stage. This is not in the official
    // apache documentation. There is no guarantee that we will instrument a module
    // if it also does this +2/-2 trick.
    //------------------------------------------------------------------------------

    // request_begin and request_end hooks
    ap_hook_header_parser(ApacheHooks::appd_hook_header_parser_begin, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_log_transaction(ApacheHooks::appd_hook_log_transaction_end, NULL, NULL, APR_HOOK_REALLY_LAST + 2);

    // Register the output filter after the response resource is generated (AP_FTYPE_CONTENT_SET).
    // The filter gets added into the chain in appd_hook_interaction_end_insert_filter via ap_hook_insert_filter.
    ap_register_output_filter(APPD_OUTPUT_FILTER_NAME, ApacheHooks::appd_output_filter, NULL, AP_FTYPE_CONTENT_SET);
    ap_hook_insert_filter(ApacheHooks::appd_hook_interaction_end_insert_filter, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);

    // TODO: For the time being put in a hook at the very beginning of every stage we are instrumenting. This is a
    // catch all to stop a interaction. We can be smarter and only put it in when we know that there is a
    // module we care about for a previous stage. This means we need to know the deterministic stage order.
    ap_hook_quick_handler(ApacheHooks::appd_hook_interaction_end_quick_handler, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_access_checker(ApacheHooks::appd_hook_interaction_end, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_check_user_id(ApacheHooks::appd_hook_interaction_end, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_auth_checker(ApacheHooks::appd_hook_interaction_end, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_type_checker(ApacheHooks::appd_hook_interaction_end, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_fixups(ApacheHooks::appd_hook_interaction_end, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_handler(ApacheHooks::appd_hook_interaction_end, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_log_transaction(ApacheHooks::appd_hook_interaction_end_log_transaction, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);

    // Stage Hooks
    // TODO: Decide among the following stages at what all we need the modules to be instrumented,
    //       and define the hooks handlers for the same to start an interaction before module callback.
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_header_parser,
            ap_hook_header_parser,
            ApacheHooksForStage::appd_header_parser_hooks,
            ApacheHooksForStage::appd_header_parser_indexes,
            ApacheHooks::appd_hook_interaction_end,
            "header_parser");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_quick_handler,
            ap_hook_quick_handler,
            ApacheHooksForStage::appd_quick_handler_hooks,
            ApacheHooksForStage::appd_quick_handler_indexes,
            ApacheHooks::appd_hook_interaction_end_quick_handler,
            "quick_handler");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_access_checker,
            ap_hook_access_checker,
            ApacheHooksForStage::appd_access_checker_hooks,
            ApacheHooksForStage::appd_access_checker_indexes,
            ApacheHooks::appd_hook_interaction_end,
            "access_checker");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_check_user_id,
            ap_hook_check_user_id,
            ApacheHooksForStage::appd_check_user_id_hooks,
            ApacheHooksForStage::appd_check_user_id_indexes,
            ApacheHooks::appd_hook_interaction_end,
            "check_user_id");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_auth_checker,
            ap_hook_auth_checker,
            ApacheHooksForStage::appd_auth_checker_hooks,
            ApacheHooksForStage::appd_auth_checker_indexes,
            ApacheHooks::appd_hook_interaction_end,
            "auth_checker");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_type_checker,
            ap_hook_type_checker,
            ApacheHooksForStage::appd_type_checker_hooks,
            ApacheHooksForStage::appd_type_checker_indexes,
            ApacheHooks::appd_hook_interaction_end,
            "type_checker");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_fixups,
            ap_hook_fixups,
            ApacheHooksForStage::appd_fixups_hooks,
            ApacheHooksForStage::appd_fixups_indexes,
            ApacheHooks::appd_hook_interaction_end,
            "fixups");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_insert_filter,
            ap_hook_insert_filter,
            ApacheHooksForStage::appd_insert_filter_hooks,
            ApacheHooksForStage::appd_insert_filter_indexes,
            ApacheHooks::appd_hook_interaction_end_insert_filter,
            "insert_filter");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_handler,
            ap_hook_handler,
            ApacheHooksForStage::appd_handler_hooks,
            ApacheHooksForStage::appd_handler_indexes,
            ApacheHooks::appd_hook_interaction_end_handler,
            "handler");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_log_transaction,
            ap_hook_log_transaction,
            ApacheHooksForStage::appd_log_transaction_hooks,
            ApacheHooksForStage::appd_log_transaction_indexes,
            ApacheHooks::appd_hook_interaction_end,
            "log_transaction");
}

apr_status_t ApacheHooks::appd_output_filter(ap_filter_t* f, apr_bucket_brigade* bb)
{
    if (APR_BRIGADE_EMPTY(bb))
    {
        // "An output filter should never pass an empty brigade down the filter chain."
        // https://httpd.apache.org/docs/trunk/developer/output-filters.html
        return APR_SUCCESS;
    }

    /*
        Perform the task for output filter particularly of EUM Auto-injection.
    */

    // Remove ourselves from the chain and pass everything down
    ap_remove_output_filter(f);
    return ap_pass_brigade(f->next, bb);
}

// end the interaction
void ApacheHooks::appd_stopInteraction(request_rec *r, bool isAlwaysRunStage, bool ignoreBackend)
{
    if ((!isAlwaysRunStage && !m_reportAllStages) || !r || !r->notes || !ap_is_initial_req(r))
    {
        return;
    }
    APPD_SDK_HANDLE_REQ reqHandlePtr = (APPD_SDK_HANDLE_REQ)apr_table_get(r->notes, APPD_REQ_HANDLE_KEY);
    if (reqHandlePtr)
    {
        // TODO: Work on backend naming and type
        std::string backendName;
        std::string backendType = "HTTP"; // temporary
        long errorCode = r->status;
        std::ostringstream oss("");

        if (appd_requestHasErrors(r))
        {
            oss << "HTTP ERROR CODE:" << r->status;
        }

        std::unique_ptr<appd::core::EndInteractionPayload> payload(new
            appd::core::EndInteractionPayload(backendName, backendType, errorCode, oss.str()));
        APPD_SDK_STATUS_CODE res = wsAgent.endInteraction(reqHandlePtr, ignoreBackend, payload.get());

        if (APPD_ISFAIL(res))
        {
            ApacheTracing::writeTrace(r->server, __func__, "result code: %d", res);
        }
    }
}

// stop endpoint interaction if one currently is running
// begin the new interaction
APPD_SDK_STATUS_CODE ApacheHooks::appd_startInteraction(
        request_rec* r,
        HookContainer::appd_endpoint_indexes endpointIndex,
        bool isAlwaysRunStage,
        bool ignoreBackend)
{
    /*
        Retrieve the stage and module name from endpointIndex and start an interaction to module.
    */
    APPD_SDK_STATUS_CODE res = APPD_SUCCESS;
    if ((!isAlwaysRunStage && !m_reportAllStages) || !r || !r->notes || !ap_is_initial_req(r))
    {
        return res;
    }

    const std::string& stage = HookContainer::getInstance().getStage(endpointIndex);
    const std::string& module = HookContainer::getInstance().getModule(endpointIndex);

    APPD_SDK_HANDLE_REQ reqHandlePtr = (APPD_SDK_HANDLE_REQ)apr_table_get(r->notes, APPD_REQ_HANDLE_KEY);
    if (reqHandlePtr)
    {
        // In case a previous "end hook" is never called. End current interaction before starting new one
        appd_stopInteraction(r, isAlwaysRunStage, ignoreBackend);

        bool resolveBackends = false;
        appd_cfg* cfg = ApacheConfigHandlers::getConfig(r);
        if (cfg)
        {
            resolveBackends = cfg->getResolveBackends();
        }

        std::unique_ptr<appd::core::InteractionPayload> payload(new
            appd::core::InteractionPayload(module, stage, resolveBackends));

        // Create propagationHeaders to be populated in startInteraction.
        std::unordered_map<std::string, std::string> propagationHeaders;
        res = wsAgent.startInteraction(reqHandlePtr, payload.get(), propagationHeaders);

        if (APPD_ISSUCCESS(res))
        {
            if (!propagationHeaders.empty())
            {
                appd_payload_decorator(r, propagationHeaders);
            }
            else
            {
                ApacheTracing::writeTrace(r->server, module.c_str(),
                    "propagationHeaders were not filled, cannot add correlation information into request headers");
            }
            ApacheTracing::writeTrace(r->server, module.c_str(), "interaction begin successful");
        }
        else
        {
            ApacheTracing::writeTrace(r->server, module.c_str(), "Error: interaction begin result code: %d", res);
        }
    }
    return res;
}

//-----------------------------------------------------------------------------------------
// Backend Payload Decorator
// Used for following purposes:
//    1) adding the correlation for this backend request to the headers
//-----------------------------------------------------------------------------------------
void ApacheHooks::appd_payload_decorator(request_rec* request, std::unordered_map<std::string, std::string> propagationHeaders)
{
    for (auto itr = propagationHeaders.begin(); itr != propagationHeaders.end(); itr++)
    {
        // put the correlation information into the http headers
        apr_table_set(request->headers_in, itr->first.c_str(), itr->second.c_str());

        ApacheTracing::writeTrace(request->server, __func__,
                "correlation information : \"%s\" : \"%s\"",
                itr->first.c_str(),
                itr->second.c_str());
    }
}

// Utility routine for initializing the SDK based on configuration parameters
bool ApacheHooks::initialize_appdynamics(const request_rec *r)
{
    // check to see if we have already been initialized by getting user data from the process pool
    void* config_data = NULL;
    apr_pool_userdata_get(&config_data, APPD_CONFIG_KEY, r->server->process->pool);

    if (config_data != NULL)
    {
        ApacheTracing::writeTrace(r->server, __func__, "config retrieved from process memory pool");
        return true;
    }

    // we don't have a config stored in the process memory pool
    // try to initialize and store a config in the process memory pool
    ApacheTracing::writeTrace(r->server, __func__, "no config stored in the process memory pool");

    appd_cfg* our_config = ApacheConfigHandlers::getConfig(r);
    if (our_config == NULL)
    {
        ApacheTracing::writeTrace(r->server, __func__, "config was NULL");
        return false;
    }
    // log config
    ApacheConfigHandlers::traceConfig(r, our_config);

    // Check to see if the agent is enabled
    if (our_config->getAppdEnabled() == 1)
    {
        // Intialize the SDK with our configuration information
        APPD_SDK_STATUS_CODE res = APPD_SUCCESS;

        wsAgent.initDependency();

        // ENV RECORDS SIZE TO INCLUDE THE LOG PATH AND THE AGGREGATOR DIRECTORY
        //
        // Update the apr_pcalloc if we add another parameter to the input array!
        APPD_SDK_ENV_RECORD* env_config =
                (APPD_SDK_ENV_RECORD*) apr_pcalloc(r->pool, 16 * sizeof(APPD_SDK_ENV_RECORD));
        int ix = 0;

        // Otel Exporter Type
        env_config[ix].name = APPD_SDK_ENV_OTEL_EXPORTER_TYPE;
        env_config[ix].value = our_config->getOtelExporterType();
        ++ix;

        // Otel Exporter Endpoint
        env_config[ix].name = APPD_SDK_ENV_OTEL_EXPORTER_ENDPOINT;
        env_config[ix].value = our_config->getOtelExporterEndpoint();
        ++ix;

        // Otel SSL Enabled
        env_config[ix].name = APPD_SDK_ENV_OTEL_SSL_ENABLED;
        env_config[ix].value = our_config->getOtelSslEnabled() == 1 ? "1" : "0";
        ++ix;

        // Otel Certificate Path
        env_config[ix].name = APPD_SDK_ENV_OTEL_SSL_CERTIFICATE_PATH;
        env_config[ix].value = our_config->getOtelSslCertificatePath();
        ++ix;

        // sdk libaray name
        env_config[ix].name = APPD_SDK_ENV_OTEL_LIBRARY_NAME;
        env_config[ix].value = "Apache";
        ++ix;

        // Otel Processor Type
        env_config[ix].name = APPD_SDK_ENV_OTEL_PROCESSOR_TYPE;
        env_config[ix].value = our_config->getOtelProcessorType();
        ++ix;

        // Otel Sampler Type
        env_config[ix].name = APPD_SDK_ENV_OTEL_SAMPLER_TYPE;
        env_config[ix].value = our_config->getOtelSamplerType();
        ++ix;

        // Service Namespace
        env_config[ix].name = APPD_SDK_ENV_SERVICE_NAMESPACE;
        env_config[ix].value = our_config->getServiceNamespace();
        ++ix;

        // Service Name
        env_config[ix].name = APPD_SDK_ENV_SERVICE_NAME;
        env_config[ix].value = our_config->getServiceName();
        ++ix;

        // Service Instance ID
        env_config[ix].name = APPD_SDK_ENV_SERVICE_INSTANCE_ID;
        env_config[ix].value = our_config->getServiceInstanceId();
        ++ix;

        // Otel Max Queue Size
        env_config[ix].name = APPD_SDK_ENV_MAX_QUEUE_SIZE;
        env_config[ix].value = our_config->getOtelMaxQueueSize();
        ++ix;

        // Otel Scheduled Delay
        env_config[ix].name = APPD_SDK_ENV_SCHEDULED_DELAY;
        env_config[ix].value = our_config->getOtelScheduledDelay();
        ++ix;

        // Otel Max Export Batch Size
        env_config[ix].name = APPD_SDK_ENV_EXPORT_BATCH_SIZE;
        env_config[ix].value = our_config->getOtelMaxExportBatchSize();
        ++ix;

        // Otel Export Timeout
        env_config[ix].name = APPD_SDK_ENV_EXPORT_TIMEOUT;
        env_config[ix].value = our_config->getOtelExportTimeout();
        ++ix;

        // Segment Type
        env_config[ix].name = APPD_SDK_ENV_SEGMENT_TYPE;
        env_config[ix].value = our_config->getSegmentType();
        ++ix;

        // Segment Parameter
        env_config[ix].name = APPD_SDK_ENV_SEGMENT_PARAMETER;
        env_config[ix].value = our_config->getSegmentParameter();
        ++ix;

        // !!!
        // Remember to update the apr_pcalloc call size if we add another parameter to the input array!
        // !!!

        for (auto it = ApacheConfigHandlers::m_webServerContexts.begin(); it != ApacheConfigHandlers::m_webServerContexts.end(); ++it)
        {
            appd::core::WSContextConfig cfg;
            cfg.serviceNamespace = it->second->m_serviceNamespace;
            cfg.serviceName = it->second->m_serviceName;
            cfg.serviceInstanceId = it->second->m_serviceInstanceId;

            wsAgent.addWSContextToCore(it->first.c_str(), &cfg);
            ApacheTracing::writeTrace(r->server, __func__, "Adding Context: %s ServiceNamespace: %s ServiceName: %s ServiceInstanceId: %s",
                it->first.c_str(),
                it->second->m_serviceNamespace.c_str(),
                it->second->m_serviceName.c_str(),
                it->second->m_serviceInstanceId.c_str());
        }

        res = wsAgent.init(env_config, ix);
        if (APPD_ISSUCCESS(res))
        {
            ApacheTracing::writeTrace(r->server, __func__, "initializing SDK succceeded");

            // Store the config that was selected for all other methods to be able to use
            appd_cfg* process_cfg = ApacheConfigHandlers::getProcessConfig(r);
            apr_pool_userdata_set((const void *)process_cfg, APPD_CONFIG_KEY, apr_pool_cleanup_null, r->server->process->pool);

            return true;
        }
        else
        {
            ApacheTracing::writeError(r->server, __func__, "SDK Init failed, result code is: %d", res);
            return false;
        }
    }
    else
    {
        ApacheTracing::writeError(r->server, __func__, "agent not enabled");
        return false;
    }
    return false;
}

bool ApacheHooks::appd_requestHasErrors(request_rec* r)
{
    return r->status >= LOWEST_HTTP_ERROR_CODE;
}

void fillRequestPayload(request_rec* request, appd::core::RequestPayload* payload)
{
    const char* val;

    // httpHeaders
    for (auto itr = ApacheHooks::httpHeaders.begin(); itr !=  ApacheHooks::httpHeaders.end(); ++itr)
    {
        const char* hdr = *itr;
        val = apr_table_get(request->headers_in, hdr);

        if (!val)
        {
            val = " ";
        }
        else
        {
            payload->set_http_headers(hdr, val);
        }
    }

    // uri
    val = request->parsed_uri.path ? request->parsed_uri.path : " ";
    payload->set_uri(val);

    // request_protocol
    val = request->protocol ? request->protocol : " ";
    payload->set_request_protocol(val);

    // http_post_parameter
    if (!request->method || !request->args || apr_strnatcmp(request->method, "POST"))
    {
        val = " ";
    }
    else val = request->args;
    payload->set_http_post_parameter(val);

    // http_get_parameter
    if(!request->method || !request->args || apr_strnatcmp(request->method, "GET"))
    {
        val = " ";
    }
    else val = request->args;
    payload->set_http_get_parameter(val);

    // http_request_method
    val = request->method ? request->method: " ";
    payload->set_http_request_method(val);

    // websrv-698 Setting server span attributes
    payload->set_status_code(request->status);

    if (request->server)
    {
        payload->set_server_name(request->server->server_hostname);
    }
    payload->set_scheme(ap_run_http_scheme(request));

    if (request->hostname)
    {
        payload->set_host(request->hostname);
    }

    if (request->unparsed_uri)
    {
        payload->set_target(request->unparsed_uri);
    }

    switch (request->proto_num)
    {  // TODO: consider using ap_get_protocol for other flavors
      case HTTP_PROTO_1000:
        payload->set_flavor(kHTTPFlavor1_0.c_str());
        break;
      case HTTP_PROTO_1001:
        payload->set_flavor(kHTTPFlavor1_1.c_str());
        break;
    }

#ifdef APLOG_USE_MODULE
    payload->set_client_ip(request->useragent_ip);
#else
    if (request->connection)
    {
        payload->set_client_ip(request->connection->remote_ip);
    }

#endif

    payload->set_port(ap_default_port(request));
}
// We have to use this hook if we want to use directory level configuration.
// post_read_request is too early in the cycle to get the config parameters for a
// directory configuration.
//
// This is a RUN_ALL hook.
int ApacheHooks::appd_hook_header_parser_begin(request_rec *r)
{
    if (!ap_is_initial_req(r))
    {
        return DECLINED;
    }
    else if (!initialize_appdynamics(r))
    {
        ApacheTracing::writeTrace(r->server, __func__, "appdynamics did not get initialized");
        return DECLINED;
    }

    // TODO: Check whether it is static page or not i.e. Excluded Extensions.

    HookContainer::getInstance().traceHooks(r);

    APPD_SDK_STATUS_CODE res = APPD_SUCCESS;
    APPD_SDK_HANDLE_REQ reqHandle = APPD_SDK_NO_HANDLE;

    const char* wscontext = NULL;
    appd_cfg* cfg = ApacheConfigHandlers::getConfig(r);
    std::string tmpContextName;

    if (cfg)
    {
        tmpContextName = ApacheConfigHandlers::computeContextName(cfg);

        if (!tmpContextName.empty())
        {
            wscontext = tmpContextName.c_str();
        }
    }

    ApacheTracing::writeTrace(r->server, __func__, "%s", (wscontext ? wscontext : "using default context"));

    // Fill the Request payload information
    std::unique_ptr<appd::core::RequestPayload> requestPayload(new appd::core::RequestPayload);
    fillRequestPayload(r, requestPayload.get());

    // Start Request
    res = wsAgent.startRequest(wscontext, requestPayload.get(), &reqHandle);

    // Store the request data into pool
    if (APPD_ISSUCCESS(res))
    {
        // Store the Request Handle on the request object
        APPD_SDK_HANDLE_REQ reqHandleValue = (APPD_SDK_HANDLE_REQ)apr_palloc(r->pool, sizeof(APPD_SDK_HANDLE_REQ));

        if (reqHandleValue)
        {
            reqHandleValue = reqHandle;
            apr_table_setn(r->notes, APPD_REQ_HANDLE_KEY, (const char*)reqHandleValue);
        }

        ApacheTracing::writeTrace(r->server, __func__, "request begin successful");
    }
    else if (res == APPD_STATUS(cfg_channel_uninitialized) || res == APPD_STATUS(bt_detection_disabled))
    {
        ApacheTracing::writeTrace(r->server, __func__, "request begin detection disabled, result code: %d", res);
        apr_table_set(r->headers_in, "singularityheader", "notxdetect=true");
    }
    else
    {
        ApacheTracing::writeTrace(r->server, __func__, "request begin error, result code: %d", res);
    }

    return DECLINED;
}

int ApacheHooks::appd_hook_log_transaction_end(request_rec* r)
{
    /*
        End the request and report the associated metrics.
    */

    if (!ap_is_initial_req(r))
    {
        return DECLINED;
    }

    // TODO - Determine whether there were any errors from any other part of this request processing (even prior to our handler)

    appd_cfg* cfg = NULL;
    apr_pool_userdata_get((void**)&cfg, APPD_CONFIG_KEY, r->server->process->pool);

    if (!cfg)
    {
        ApacheTracing::writeTrace(r->server, __func__, "configuration is NULL");
        return DECLINED;
    }
    else if (!cfg->getAppdEnabled())
    {
        ApacheTracing::writeError(r->server, __func__, "agent is disabled");
        return DECLINED;
    }

    appd_stopInteraction(r);

    APPD_SDK_HANDLE_REQ reqHandle = (APPD_SDK_HANDLE_REQ) apr_table_get(r->notes, APPD_REQ_HANDLE_KEY);

    if (!reqHandle)
    {
        return DECLINED;
    }

    apr_table_unset(r->notes, APPD_REQ_HANDLE_KEY);

    APPD_SDK_STATUS_CODE res;

    if (appd_requestHasErrors(r))
    {
        std::ostringstream oss;
        oss << r->status;
        res = wsAgent.endRequest(reqHandle, oss.str().c_str());
    }
    else
    {
        res = wsAgent.endRequest(reqHandle, NULL);
    }

    if (APPD_ISSUCCESS(res))
    {
        ApacheTracing::writeTrace(r->server, __func__, "request end successful (HTTP status=%d)", r->status);
    }
    else
    {
        ApacheTracing::writeTrace(r->server, __func__, "request end result code: %d", res);
    }

    return DECLINED;
}

//--------------------------------------------------------------------------
// HANDLER HOOKS FOR STAGES OF APACHE PROCESSING
//
// Now let's declare routines for each of the callback hooks in order.
// (That's the order in which they're listed in the callback list, *not
// the order in which the server calls them!  See the command_rec
// declaration near the bottom of this file.)  Note that these may be
// called for situations that don't relate primarily to our function - in
// other words, the fixup handler shouldn't assume that the request has
// to do with "appdynamics" stuff.
//
// With the exception of the content handler, all of our routines will be
// called for each request, unless an earlier handler from another module
// aborted the sequence.
//
// There are three types of hooks (see include/ap_config.h):
//
// VOID      : No return code, run all handlers declared by any module
// RUN_FIRST : Run all handlers until one returns something other
//             than DECLINED. Hook runner result is result of last callback
// RUN_ALL   : Run all handlers until one returns something other than OK
//             or DECLINED. The hook runner returns that other value. If
//             all hooks run, the hook runner returns OK.
//
// Handlers that are declared as "int" can return the following:
//
//  OK          Handler accepted the request and did its thing with it.
//  DECLINED    Handler took no action.
//  HTTP_mumble Handler looked at request and found it wanting.
//
// See include/httpd.h for a list of HTTP_mumble status codes.  Handlers
// that are not declared as int return a valid pointer, or NULL if they
// DECLINE to handle their phase for that specific request.  Exceptions, if
// any, are noted with each routine.
//--------------------------------------------------------------------------

apr_status_t ApacheHooks::appd_child_exit(void* data)
{
    server_rec* s = static_cast<server_rec*>(data);
    const char* sname = (s->server_hostname ? s->server_hostname : "");

    void* retdata;
    apr_pool_userdata_get(&retdata, APPD_CONFIG_KEY, s->process->pool);
    appd_cfg* our_config = static_cast<appd_cfg*>(retdata);

    if (our_config)
    {
        ApacheTracing::writeError(s, __func__, "terminating agent (sname=%s)", sname);
        wsAgent.term();
    }

    return APR_SUCCESS;
}

void ApacheHooks::appd_child_init(apr_pool_t* p, server_rec* s)
{
    ApacheTracing::logStartupTrace(s);
    apr_pool_cleanup_register(p, s, appd_child_exit, appd_child_exit);
}

// Stage Hooks

int ApacheHooksForStage::appd_hook_header_parser1(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HEADER_PARSER1);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_header_parser2(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HEADER_PARSER2);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_header_parser3(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HEADER_PARSER3);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_header_parser4(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HEADER_PARSER4);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_header_parser5(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HEADER_PARSER5);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_quick_handler1(request_rec* r, int i)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_QUICK_HANDLER1);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_quick_handler2(request_rec* r, int i)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_QUICK_HANDLER2);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_quick_handler3(request_rec* r, int i)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_QUICK_HANDLER3);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_quick_handler4(request_rec* r, int i)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_QUICK_HANDLER4);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_quick_handler5(request_rec* r, int i)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_QUICK_HANDLER5);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_access_checker1(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_ACCESS_CHECKER1);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_access_checker2(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_ACCESS_CHECKER2);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_access_checker3(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_ACCESS_CHECKER3);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_access_checker4(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_ACCESS_CHECKER4);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_access_checker5(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_ACCESS_CHECKER5);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_check_user_id1(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_CHECK_USER_ID1);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_check_user_id2(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_CHECK_USER_ID2);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_check_user_id3(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_CHECK_USER_ID3);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_check_user_id4(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_CHECK_USER_ID4);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_check_user_id5(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_CHECK_USER_ID5);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_auth_checker1(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_AUTH_CHECKER1);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_auth_checker2(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_AUTH_CHECKER2);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_auth_checker3(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_AUTH_CHECKER3);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_auth_checker4(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_AUTH_CHECKER4);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_auth_checker5(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_AUTH_CHECKER5);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_type_checker1(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_TYPE_CHECKER1);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_type_checker2(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_TYPE_CHECKER2);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_type_checker3(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_TYPE_CHECKER3);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_type_checker4(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_TYPE_CHECKER4);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_type_checker5(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_TYPE_CHECKER5);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_fixups1(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_FIXUPS1);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_fixups2(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_FIXUPS2);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_fixups3(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_FIXUPS3);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_fixups4(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_FIXUPS4);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_fixups5(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_FIXUPS5);
    return DECLINED;
}

void ApacheHooksForStage::appd_hook_insert_filter1(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_INSERT_FILTER1);
}

void ApacheHooksForStage::appd_hook_insert_filter2(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_INSERT_FILTER2);
}


void ApacheHooksForStage::appd_hook_insert_filter3(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_INSERT_FILTER3);
}

void ApacheHooksForStage::appd_hook_insert_filter4(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_INSERT_FILTER4);
}

void ApacheHooksForStage::appd_hook_insert_filter5(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_INSERT_FILTER5);
}

// The HANDLER stage does not always run all the hooks.  After a hook returns "OK", the stage
// will end and the remaining hooks are ignored. We use this behavior to ignore any backend calls
// that do not go out to another tier. Thus, if we hit any of the backend_end hook in HANDLER,
// we ignore that backend call in the SDK.
//
// ASSUMPTION: if a module returns DECLINED, it does not go out to another tier.
int ApacheHooksForStage::appd_hook_handler1(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER1,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler2(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER2,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler3(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER3,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler4(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER4,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler5(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER5,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler6(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER6,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler7(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER7,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler8(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER8,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler9(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER9,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler10(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER10,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler11(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER11,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler12(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER12,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler13(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER13,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler14(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER14,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler15(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER15,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler16(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER16,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler17(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER17,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler18(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER18,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler19(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER19,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_handler20(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_HANDLER20,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_log_transaction1(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_LOG_TRANSACTION1);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_log_transaction2(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_LOG_TRANSACTION2);
    return DECLINED;
}

int ApacheHooksForStage::appd_hook_log_transaction3(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_LOG_TRANSACTION3);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_log_transaction4(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_LOG_TRANSACTION4);
    return DECLINED;
}
int ApacheHooksForStage::appd_hook_log_transaction5(request_rec* r)
{
    ApacheHooks::appd_startInteraction(
            r,
            HookContainer::APPD_ENDPOINT_LOG_TRANSACTION5);
    return DECLINED;
}

// These hooks are for stopping interactions after a module
int ApacheHooks::appd_hook_interaction_end(request_rec *r)
{
    appd_stopInteraction(r);
    return DECLINED;
}

// The HANDLER stage does not always run all the hooks.  After a hook returns "OK", the stage
// will end and the remaining hooks are ignored. We use this behavior to ignore any endpoint calls
// that do not go out to another tier. Thus, if we hit any of the interaction_end hook in HANDLER,
// we ignore that endpoint call in the SDK.
//
// ASSUMPTION: if a module returns DECLINED, it does not go out to another tier.
int ApacheHooks::appd_hook_interaction_end_handler(request_rec *r)
{
    appd_stopInteraction(r, true, true); // always run and ignore backend if triggered
    return DECLINED;
}

int ApacheHooks::appd_hook_interaction_end_quick_handler(request_rec *r, int i)
{
    appd_stopInteraction(r);
    return DECLINED;
}

void ApacheHooks::appd_hook_interaction_end_insert_filter(request_rec *r)
{
    // Add our output filter into the chain for this request.
    ap_add_output_filter(APPD_OUTPUT_FILTER_NAME, NULL, r, r->connection);
    appd_stopInteraction(r);
}

int ApacheHooks::appd_hook_interaction_end_log_transaction(request_rec *r)
{
    appd_stopInteraction(r, true, false); // always run and do not ignore backends
    return DECLINED;
}

template<typename T, typename S>
void ApacheHooksForStage::insertHooksForStage(
        apr_pool_t* p,
        hook_get_t getModules,
        T setHook,
        const std::vector<S> &beginHandlers,
        const std::vector<HookContainer::appd_endpoint_indexes> &indexes,
        S endHandler,
        const std::string& stage)
{
    std::vector<HookInfo> hooksNeeded;
    ExcludedModules::findHookPoints(hooksNeeded, getModules, stage);

    size_t hookCount = 0;

    for (auto it = hooksNeeded.begin(); it != hooksNeeded.end(); it++)
    {
        if (hookCount < beginHandlers.size())
        {
            HookContainer::getInstance().addHook(indexes[hookCount], stage, it->module, it->order);
            const char* const* moduleList = HookContainer::getInstance().getModuleList(indexes[hookCount]);

            setHook(beginHandlers[hookCount], NULL, moduleList, it->order);
            setHook(endHandler, moduleList, NULL, it->order);
            hookCount++;

            ApacheTracing::writeTrace(NULL, __func__,
                    "Instrumentation Hooks added. Stage: %s Module: %s",
                    stage.c_str(), it->module.c_str());
        }
        else
        {
            ApacheTracing::writeTrace(NULL, __func__,
                    "Instrumentation Hooks NOT added. Max hooks for stage. Stage: %s Module: %s",
                    stage.c_str(), it->module.c_str());
        }
    }
}

const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::appd_header_parser_hooks =
        {appd_hook_header_parser1
        ,appd_hook_header_parser2
        ,appd_hook_header_parser3
        ,appd_hook_header_parser4
        ,appd_hook_header_parser5};
const std::vector<HookContainer::appd_endpoint_indexes> ApacheHooksForStage::appd_header_parser_indexes =
        {HookContainer::APPD_ENDPOINT_HEADER_PARSER1
        ,HookContainer::APPD_ENDPOINT_HEADER_PARSER2
        ,HookContainer::APPD_ENDPOINT_HEADER_PARSER3
        ,HookContainer::APPD_ENDPOINT_HEADER_PARSER4
        ,HookContainer::APPD_ENDPOINT_HEADER_PARSER5};
const std::vector<ApacheHooksForStage::quickHandlerHooks> ApacheHooksForStage::appd_quick_handler_hooks =
        {appd_hook_quick_handler1
        ,appd_hook_quick_handler2
        ,appd_hook_quick_handler3
        ,appd_hook_quick_handler4
        ,appd_hook_quick_handler5};
const std::vector<HookContainer::appd_endpoint_indexes> ApacheHooksForStage::appd_quick_handler_indexes =
        {HookContainer::APPD_ENDPOINT_QUICK_HANDLER1
        ,HookContainer::APPD_ENDPOINT_QUICK_HANDLER2
        ,HookContainer::APPD_ENDPOINT_QUICK_HANDLER3
        ,HookContainer::APPD_ENDPOINT_QUICK_HANDLER4
        ,HookContainer::APPD_ENDPOINT_QUICK_HANDLER5};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::appd_access_checker_hooks =
        {appd_hook_access_checker1
        ,appd_hook_access_checker2
        ,appd_hook_access_checker3
        ,appd_hook_access_checker4
        ,appd_hook_access_checker5};
const std::vector<HookContainer::appd_endpoint_indexes> ApacheHooksForStage::appd_access_checker_indexes =
        {HookContainer::APPD_ENDPOINT_ACCESS_CHECKER1
        ,HookContainer::APPD_ENDPOINT_ACCESS_CHECKER2
        ,HookContainer::APPD_ENDPOINT_ACCESS_CHECKER3
        ,HookContainer::APPD_ENDPOINT_ACCESS_CHECKER4
        ,HookContainer::APPD_ENDPOINT_ACCESS_CHECKER5};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::appd_check_user_id_hooks =
        {appd_hook_check_user_id1
        ,appd_hook_check_user_id2
        ,appd_hook_check_user_id3
        ,appd_hook_check_user_id4
        ,appd_hook_check_user_id5};
const std::vector<HookContainer::appd_endpoint_indexes> ApacheHooksForStage::appd_check_user_id_indexes =
        {HookContainer::APPD_ENDPOINT_CHECK_USER_ID1
        ,HookContainer::APPD_ENDPOINT_CHECK_USER_ID2
        ,HookContainer::APPD_ENDPOINT_CHECK_USER_ID3
        ,HookContainer::APPD_ENDPOINT_CHECK_USER_ID4
        ,HookContainer::APPD_ENDPOINT_CHECK_USER_ID5};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::appd_auth_checker_hooks =
        {appd_hook_auth_checker1
        ,appd_hook_auth_checker2
        ,appd_hook_auth_checker3
        ,appd_hook_auth_checker4
        ,appd_hook_auth_checker5};
const std::vector<HookContainer::appd_endpoint_indexes> ApacheHooksForStage::appd_auth_checker_indexes =
        {HookContainer::APPD_ENDPOINT_AUTH_CHECKER1
        ,HookContainer::APPD_ENDPOINT_AUTH_CHECKER2
        ,HookContainer::APPD_ENDPOINT_AUTH_CHECKER3
        ,HookContainer::APPD_ENDPOINT_AUTH_CHECKER4
        ,HookContainer::APPD_ENDPOINT_AUTH_CHECKER5};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::appd_type_checker_hooks =
        {appd_hook_type_checker1
        ,appd_hook_type_checker2
        ,appd_hook_type_checker3
        ,appd_hook_type_checker4
        ,appd_hook_type_checker5};
const std::vector<HookContainer::appd_endpoint_indexes> ApacheHooksForStage::appd_type_checker_indexes =
        {HookContainer::APPD_ENDPOINT_TYPE_CHECKER1
        ,HookContainer::APPD_ENDPOINT_TYPE_CHECKER2
        ,HookContainer::APPD_ENDPOINT_TYPE_CHECKER3
        ,HookContainer::APPD_ENDPOINT_TYPE_CHECKER4
        ,HookContainer::APPD_ENDPOINT_TYPE_CHECKER5};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::appd_fixups_hooks =
        {appd_hook_fixups1
        ,appd_hook_fixups2
        ,appd_hook_fixups3
        ,appd_hook_fixups4
        ,appd_hook_fixups5};
const std::vector<HookContainer::appd_endpoint_indexes> ApacheHooksForStage::appd_fixups_indexes =
        {HookContainer::APPD_ENDPOINT_FIXUPS1
        ,HookContainer::APPD_ENDPOINT_FIXUPS2
        ,HookContainer::APPD_ENDPOINT_FIXUPS3
        ,HookContainer::APPD_ENDPOINT_FIXUPS4
        ,HookContainer::APPD_ENDPOINT_FIXUPS5};
const std::vector<ApacheHooksForStage::filterHooks> ApacheHooksForStage::appd_insert_filter_hooks =
        {appd_hook_insert_filter1
        ,appd_hook_insert_filter2
        ,appd_hook_insert_filter3
        ,appd_hook_insert_filter4
        ,appd_hook_insert_filter5};
const std::vector<HookContainer::appd_endpoint_indexes> ApacheHooksForStage::appd_insert_filter_indexes =
        {HookContainer::APPD_ENDPOINT_INSERT_FILTER1
        ,HookContainer::APPD_ENDPOINT_INSERT_FILTER2
        ,HookContainer::APPD_ENDPOINT_INSERT_FILTER3
        ,HookContainer::APPD_ENDPOINT_INSERT_FILTER4
        ,HookContainer::APPD_ENDPOINT_INSERT_FILTER5};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::appd_handler_hooks =
        {appd_hook_handler1
        ,appd_hook_handler2
        ,appd_hook_handler3
        ,appd_hook_handler4
        ,appd_hook_handler5
        ,appd_hook_handler6
        ,appd_hook_handler7
        ,appd_hook_handler8
        ,appd_hook_handler9
        ,appd_hook_handler10
        ,appd_hook_handler11
        ,appd_hook_handler12
        ,appd_hook_handler13
        ,appd_hook_handler14
        ,appd_hook_handler15
        ,appd_hook_handler16
        ,appd_hook_handler17
        ,appd_hook_handler18
        ,appd_hook_handler19
        ,appd_hook_handler20};
const std::vector<HookContainer::appd_endpoint_indexes> ApacheHooksForStage::appd_handler_indexes =
        {HookContainer::APPD_ENDPOINT_HANDLER1
        ,HookContainer::APPD_ENDPOINT_HANDLER2
        ,HookContainer::APPD_ENDPOINT_HANDLER3
        ,HookContainer::APPD_ENDPOINT_HANDLER4
        ,HookContainer::APPD_ENDPOINT_HANDLER5
        ,HookContainer::APPD_ENDPOINT_HANDLER6
        ,HookContainer::APPD_ENDPOINT_HANDLER7
        ,HookContainer::APPD_ENDPOINT_HANDLER8
        ,HookContainer::APPD_ENDPOINT_HANDLER9
        ,HookContainer::APPD_ENDPOINT_HANDLER10
        ,HookContainer::APPD_ENDPOINT_HANDLER11
        ,HookContainer::APPD_ENDPOINT_HANDLER12
        ,HookContainer::APPD_ENDPOINT_HANDLER13
        ,HookContainer::APPD_ENDPOINT_HANDLER14
        ,HookContainer::APPD_ENDPOINT_HANDLER15
        ,HookContainer::APPD_ENDPOINT_HANDLER16
        ,HookContainer::APPD_ENDPOINT_HANDLER17
        ,HookContainer::APPD_ENDPOINT_HANDLER18
        ,HookContainer::APPD_ENDPOINT_HANDLER19
        ,HookContainer::APPD_ENDPOINT_HANDLER20};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::appd_log_transaction_hooks =
        {appd_hook_log_transaction1
        ,appd_hook_log_transaction2
        ,appd_hook_log_transaction3
        ,appd_hook_log_transaction4
        ,appd_hook_log_transaction5};
const std::vector<HookContainer::appd_endpoint_indexes> ApacheHooksForStage::appd_log_transaction_indexes =
        {HookContainer::APPD_ENDPOINT_LOG_TRANSACTION1
        ,HookContainer::APPD_ENDPOINT_LOG_TRANSACTION2
        ,HookContainer::APPD_ENDPOINT_LOG_TRANSACTION3
        ,HookContainer::APPD_ENDPOINT_LOG_TRANSACTION4
        ,HookContainer::APPD_ENDPOINT_LOG_TRANSACTION5};
