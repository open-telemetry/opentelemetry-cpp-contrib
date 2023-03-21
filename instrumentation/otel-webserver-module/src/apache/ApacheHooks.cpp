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
const char* ApacheHooks::OTEL_CONFIG_KEY = "otel_configuration";
const char* ApacheHooks::OTEL_CORRELATION_HEADER_KEY = "singularityheader";
const char* ApacheHooks::OTEL_INTERACTION_HANDLE_KEY = "otel_interaction_handle_key";
const char* ApacheHooks::OTEL_REQ_HANDLE_KEY = "otel_req_handle_key";
const char* OTEL_OUTPUT_FILTER_NAME = "OTEL_EUM_AUTOINJECT";

otel::core::WSAgent wsAgent; // global variable for interface between Hooks and Core Logic

using namespace otel::core::sdkwrapper;

void ApacheHooks::registerHooks(apr_pool_t *p)
{
    // Place a hook that executes when a child process is spawned
    // (commonly used for initializing modules after the server has forked)
    ap_hook_child_init(otel_child_init, NULL, NULL, APR_HOOK_MIDDLE);

    //------------------------------------------------------------------------------
    // The -2 and +2 below is a trick to ensure that our hooks are before and after
    // the majority of the modules within a stage. This is not in the official
    // apache documentation. There is no guarantee that we will instrument a module
    // if it also does this +2/-2 trick.
    //------------------------------------------------------------------------------

    // request_begin and request_end hooks
    ap_hook_header_parser(ApacheHooks::otel_hook_header_parser_begin, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_log_transaction(ApacheHooks::otel_hook_log_transaction_end, NULL, NULL, APR_HOOK_REALLY_LAST + 2);

    // Register the output filter after the response resource is generated (AP_FTYPE_CONTENT_SET).
    // The filter gets added into the chain in otel_hook_interaction_end_insert_filter via ap_hook_insert_filter.
    ap_register_output_filter(OTEL_OUTPUT_FILTER_NAME, ApacheHooks::otel_output_filter, NULL, AP_FTYPE_CONTENT_SET);
    ap_hook_insert_filter(ApacheHooks::otel_hook_interaction_end_insert_filter, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);

    // TODO: For the time being put in a hook at the very beginning of every stage we are instrumenting. This is a
    // catch all to stop a interaction. We can be smarter and only put it in when we know that there is a
    // module we care about for a previous stage. This means we need to know the deterministic stage order.
    ap_hook_quick_handler(ApacheHooks::otel_hook_interaction_end_quick_handler, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_access_checker(ApacheHooks::otel_hook_interaction_end, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_check_user_id(ApacheHooks::otel_hook_interaction_end, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_auth_checker(ApacheHooks::otel_hook_interaction_end, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_type_checker(ApacheHooks::otel_hook_interaction_end, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_fixups(ApacheHooks::otel_hook_interaction_end, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_handler(ApacheHooks::otel_hook_interaction_end, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);
    ap_hook_log_transaction(ApacheHooks::otel_hook_interaction_end_log_transaction, NULL, NULL, APR_HOOK_REALLY_FIRST - 2);

    // Stage Hooks
    // TODO: Decide among the following stages at what all we need the modules to be instrumented,
    //       and define the hooks handlers for the same to start an interaction before module callback.
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_header_parser,
            ap_hook_header_parser,
            ApacheHooksForStage::otel_header_parser_hooks,
            ApacheHooksForStage::otel_header_parser_indexes,
            ApacheHooks::otel_hook_interaction_end,
            "header_parser");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_quick_handler,
            ap_hook_quick_handler,
            ApacheHooksForStage::otel_quick_handler_hooks,
            ApacheHooksForStage::otel_quick_handler_indexes,
            ApacheHooks::otel_hook_interaction_end_quick_handler,
            "quick_handler");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_access_checker,
            ap_hook_access_checker,
            ApacheHooksForStage::otel_access_checker_hooks,
            ApacheHooksForStage::otel_access_checker_indexes,
            ApacheHooks::otel_hook_interaction_end,
            "access_checker");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_check_user_id,
            ap_hook_check_user_id,
            ApacheHooksForStage::otel_check_user_id_hooks,
            ApacheHooksForStage::otel_check_user_id_indexes,
            ApacheHooks::otel_hook_interaction_end,
            "check_user_id");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_auth_checker,
            ap_hook_auth_checker,
            ApacheHooksForStage::otel_auth_checker_hooks,
            ApacheHooksForStage::otel_auth_checker_indexes,
            ApacheHooks::otel_hook_interaction_end,
            "auth_checker");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_type_checker,
            ap_hook_type_checker,
            ApacheHooksForStage::otel_type_checker_hooks,
            ApacheHooksForStage::otel_type_checker_indexes,
            ApacheHooks::otel_hook_interaction_end,
            "type_checker");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_fixups,
            ap_hook_fixups,
            ApacheHooksForStage::otel_fixups_hooks,
            ApacheHooksForStage::otel_fixups_indexes,
            ApacheHooks::otel_hook_interaction_end,
            "fixups");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_insert_filter,
            ap_hook_insert_filter,
            ApacheHooksForStage::otel_insert_filter_hooks,
            ApacheHooksForStage::otel_insert_filter_indexes,
            ApacheHooks::otel_hook_interaction_end_insert_filter,
            "insert_filter");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_handler,
            ap_hook_handler,
            ApacheHooksForStage::otel_handler_hooks,
            ApacheHooksForStage::otel_handler_indexes,
            ApacheHooks::otel_hook_interaction_end_handler,
            "handler");
    ApacheHooksForStage::insertHooksForStage(
            p,
            ap_hook_get_log_transaction,
            ap_hook_log_transaction,
            ApacheHooksForStage::otel_log_transaction_hooks,
            ApacheHooksForStage::otel_log_transaction_indexes,
            ApacheHooks::otel_hook_interaction_end,
            "log_transaction");
}

apr_status_t ApacheHooks::otel_output_filter(ap_filter_t* f, apr_bucket_brigade* bb)
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
void ApacheHooks::otel_stopInteraction(request_rec *r, bool isAlwaysRunStage, bool ignoreBackend)
{
    if ((!isAlwaysRunStage && !m_reportAllStages) || !r || !r->notes || !ap_is_initial_req(r))
    {
        return;
    }
    OTEL_SDK_HANDLE_REQ reqHandlePtr = (OTEL_SDK_HANDLE_REQ)apr_table_get(r->notes, OTEL_REQ_HANDLE_KEY);
    if (reqHandlePtr)
    {
        // TODO: Work on backend naming and type
        std::string backendName;
        std::string backendType = "HTTP"; // temporary
        long errorCode = r->status;
        std::ostringstream oss("");

        if (otel_requestHasErrors(r))
        {
            oss << "HTTP ERROR CODE:" << r->status;
        }

        std::unique_ptr<otel::core::EndInteractionPayload> payload(new
            otel::core::EndInteractionPayload(backendName, backendType, errorCode, oss.str()));
        OTEL_SDK_STATUS_CODE res = wsAgent.endInteraction(reqHandlePtr, ignoreBackend, payload.get());

        if (OTEL_ISFAIL(res))
        {
            ApacheTracing::writeTrace(r->server, __func__, "result code: %d", res);
        }
    }
}

// stop endpoint interaction if one currently is running
// begin the new interaction
OTEL_SDK_STATUS_CODE ApacheHooks::otel_startInteraction(
        request_rec* r,
        HookContainer::otel_endpoint_indexes endpointIndex,
        bool isAlwaysRunStage,
        bool ignoreBackend)
{
    /*
        Retrieve the stage and module name from endpointIndex and start an interaction to module.
    */
    OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;
    if ((!isAlwaysRunStage && !m_reportAllStages) || !r || !r->notes || !ap_is_initial_req(r))
    {
        return res;
    }

    const std::string& stage = HookContainer::getInstance().getStage(endpointIndex);
    const std::string& module = HookContainer::getInstance().getModule(endpointIndex);

    OTEL_SDK_HANDLE_REQ reqHandlePtr = (OTEL_SDK_HANDLE_REQ)apr_table_get(r->notes, OTEL_REQ_HANDLE_KEY);
    if (reqHandlePtr)
    {
        // In case a previous "end hook" is never called. End current interaction before starting new one
        otel_stopInteraction(r, isAlwaysRunStage, ignoreBackend);

        bool resolveBackends = false;
        otel_cfg* cfg = ApacheConfigHandlers::getConfig(r);
        if (cfg)
        {
            resolveBackends = cfg->getResolveBackends();
        }

        std::unique_ptr<otel::core::InteractionPayload> payload(new
            otel::core::InteractionPayload(module, stage, resolveBackends));

        // Create propagationHeaders to be populated in startInteraction.
        std::unordered_map<std::string, std::string> propagationHeaders;
        res = wsAgent.startInteraction(reqHandlePtr, payload.get(), propagationHeaders);

        if (OTEL_ISSUCCESS(res))
        {
            // remove the singularity header if any
            apr_table_unset(r->headers_in, OTEL_CORRELATION_HEADER_KEY);
            if (!propagationHeaders.empty())
            {
                otel_payload_decorator(r, propagationHeaders);
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
void ApacheHooks::otel_payload_decorator(request_rec* request, std::unordered_map<std::string, std::string> propagationHeaders)
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
bool ApacheHooks::initialize_opentelemetry(const request_rec *r)
{
    // check to see if we have already been initialized by getting user data from the process pool
    void* config_data = NULL;
    apr_pool_userdata_get(&config_data, OTEL_CONFIG_KEY, r->server->process->pool);

    if (config_data != NULL)
    {
        ApacheTracing::writeTrace(r->server, __func__, "config retrieved from process memory pool");
        return true;
    }

    // we don't have a config stored in the process memory pool
    // try to initialize and store a config in the process memory pool
    ApacheTracing::writeTrace(r->server, __func__, "no config stored in the process memory pool");

    otel_cfg* our_config = ApacheConfigHandlers::getConfig(r);
    if (our_config == NULL)
    {
        ApacheTracing::writeTrace(r->server, __func__, "config was NULL");
        return false;
    }
    // log config
    ApacheConfigHandlers::traceConfig(r, our_config);

    // Check to see if the agent is enabled
    if (our_config->getOtelEnabled() == 1)
    {
        // Intialize the SDK with our configuration information
        OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;

        wsAgent.initDependency();

        // ENV RECORDS SIZE TO INCLUDE THE LOG PATH AND THE AGGREGATOR DIRECTORY
        // Update the CONFIG_COUNT in apr_pcalloc if we add another parameter to the input array!
        
        OTEL_SDK_ENV_RECORD* env_config =
                (OTEL_SDK_ENV_RECORD*) apr_pcalloc(r->pool, CONFIG_COUNT * sizeof(OTEL_SDK_ENV_RECORD));

        int ix = 0;

        // Otel Exporter Type
        env_config[ix].name = OTEL_SDK_ENV_OTEL_EXPORTER_TYPE;
        env_config[ix].value = our_config->getOtelExporterType();
        ++ix;

        // Otel Exporter Endpoint
        env_config[ix].name = OTEL_SDK_ENV_OTEL_EXPORTER_ENDPOINT;
        env_config[ix].value = our_config->getOtelExporterEndpoint();
        ++ix;

        // Otel SSL Enabled
        env_config[ix].name = OTEL_SDK_ENV_OTEL_SSL_ENABLED;
        env_config[ix].value = our_config->getOtelSslEnabled() == 1 ? "1" : "0";
        ++ix;

        // Otel Certificate Path
        env_config[ix].name = OTEL_SDK_ENV_OTEL_SSL_CERTIFICATE_PATH;
        env_config[ix].value = our_config->getOtelSslCertificatePath();
        ++ix;

        // sdk libaray name
        env_config[ix].name = OTEL_SDK_ENV_OTEL_LIBRARY_NAME;
        env_config[ix].value = "Apache";
        ++ix;

        // Otel Processor Type
        env_config[ix].name = OTEL_SDK_ENV_OTEL_PROCESSOR_TYPE;
        env_config[ix].value = our_config->getOtelProcessorType();
        ++ix;

        // Otel Sampler Type
        env_config[ix].name = OTEL_SDK_ENV_OTEL_SAMPLER_TYPE;
        env_config[ix].value = our_config->getOtelSamplerType();
        ++ix;

        // Service Namespace
        env_config[ix].name = OTEL_SDK_ENV_SERVICE_NAMESPACE;
        env_config[ix].value = our_config->getServiceNamespace();
        ++ix;

        // Service Name
        env_config[ix].name = OTEL_SDK_ENV_SERVICE_NAME;
        env_config[ix].value = our_config->getServiceName();
        ++ix;

        // Service Instance ID
        env_config[ix].name = OTEL_SDK_ENV_SERVICE_INSTANCE_ID;
        env_config[ix].value = our_config->getServiceInstanceId();
        ++ix;

        // Otel Max Queue Size
        env_config[ix].name = OTEL_SDK_ENV_MAX_QUEUE_SIZE;
        env_config[ix].value = our_config->getOtelMaxQueueSize();
        ++ix;

        // Otel Scheduled Delay
        env_config[ix].name = OTEL_SDK_ENV_SCHEDULED_DELAY;
        env_config[ix].value = our_config->getOtelScheduledDelay();
        ++ix;

        // Otel Max Export Batch Size
        env_config[ix].name = OTEL_SDK_ENV_EXPORT_BATCH_SIZE;
        env_config[ix].value = our_config->getOtelMaxExportBatchSize();
        ++ix;

        // Otel Export Timeout
        env_config[ix].name = OTEL_SDK_ENV_EXPORT_TIMEOUT;
        env_config[ix].value = our_config->getOtelExportTimeout();
        ++ix;

        // Segment Type
        env_config[ix].name = OTEL_SDK_ENV_SEGMENT_TYPE;
        env_config[ix].value = our_config->getSegmentType();
        ++ix;

        // Segment Parameter
        env_config[ix].name = OTEL_SDK_ENV_SEGMENT_PARAMETER;
        env_config[ix].value = our_config->getSegmentParameter();
        ++ix;

        // Segment Parameter
        env_config[ix].name = OTEL_SDK_ENV_OTEL_EXPORTER_OTLPHEADERS;
        env_config[ix].value = our_config->getOtelExporterOtlpHeaders();
        ++ix;

        // !!!
        // Remember to update the apr_pcalloc call size if we add another parameter to the input array!
        // !!!

        for (auto it = ApacheConfigHandlers::m_webServerContexts.begin(); it != ApacheConfigHandlers::m_webServerContexts.end(); ++it)
        {
            otel::core::WSContextConfig cfg;
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
        if (OTEL_ISSUCCESS(res))
        {
            ApacheTracing::writeTrace(r->server, __func__, "initializing SDK succceeded");

            // Store the config that was selected for all other methods to be able to use
            otel_cfg* process_cfg = ApacheConfigHandlers::getProcessConfig(r);
            apr_pool_userdata_set((const void *)process_cfg, OTEL_CONFIG_KEY, apr_pool_cleanup_null, r->server->process->pool);

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

bool ApacheHooks::otel_requestHasErrors(request_rec* r)
{
    return r->status >= LOWEST_HTTP_ERROR_CODE;
}

void fillRequestPayload(request_rec* request, otel::core::RequestPayload* payload)
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
int ApacheHooks::otel_hook_header_parser_begin(request_rec *r)
{
    if (!ap_is_initial_req(r))
    {
        return DECLINED;
    }
    else if (!initialize_opentelemetry(r))
    {
        ApacheTracing::writeTrace(r->server, __func__, "opentelemetry did not get initialized");
        return DECLINED;
    }

    // TODO: Check whether it is static page or not i.e. Excluded Extensions.

    HookContainer::getInstance().traceHooks(r);

    OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;
    OTEL_SDK_HANDLE_REQ reqHandle = OTEL_SDK_NO_HANDLE;

    const char* wscontext = NULL;
    otel_cfg* cfg = ApacheConfigHandlers::getConfig(r);
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
    std::unique_ptr<otel::core::RequestPayload> requestPayload(new otel::core::RequestPayload);
    fillRequestPayload(r, requestPayload.get());

    // Start Request
    res = wsAgent.startRequest(wscontext, requestPayload.get(), &reqHandle);

    // Store the request data into pool
    if (OTEL_ISSUCCESS(res))
    {
        // Store the Request Handle on the request object
        OTEL_SDK_HANDLE_REQ reqHandleValue = (OTEL_SDK_HANDLE_REQ)apr_palloc(r->pool, sizeof(OTEL_SDK_HANDLE_REQ));

        if (reqHandleValue)
        {
            reqHandleValue = reqHandle;
            apr_table_setn(r->notes, OTEL_REQ_HANDLE_KEY, (const char*)reqHandleValue);
        }

        ApacheTracing::writeTrace(r->server, __func__, "request begin successful");
    }
    else if (res == OTEL_STATUS(cfg_channel_uninitialized) || res == OTEL_STATUS(bt_detection_disabled))
    {
        ApacheTracing::writeTrace(r->server, __func__, "request begin detection disabled, result code: %d", res);
    }
    else
    {
        ApacheTracing::writeTrace(r->server, __func__, "request begin error, result code: %d", res);
    }

    return DECLINED;
}

int ApacheHooks::otel_hook_log_transaction_end(request_rec* r)
{
    /*
        End the request and report the associated metrics.
    */

    if (!ap_is_initial_req(r))
    {
        return DECLINED;
    }

    // TODO - Determine whether there were any errors from any other part of this request processing (even prior to our handler)

    otel_cfg* cfg = NULL;
    apr_pool_userdata_get((void**)&cfg, OTEL_CONFIG_KEY, r->server->process->pool);

    if (!cfg)
    {
        ApacheTracing::writeTrace(r->server, __func__, "configuration is NULL");
        return DECLINED;
    }
    else if (!cfg->getOtelEnabled())
    {
        ApacheTracing::writeError(r->server, __func__, "agent is disabled");
        return DECLINED;
    }

    otel_stopInteraction(r);

    OTEL_SDK_HANDLE_REQ reqHandle = (OTEL_SDK_HANDLE_REQ) apr_table_get(r->notes, OTEL_REQ_HANDLE_KEY);

    if (!reqHandle)
    {
        return DECLINED;
    }

    apr_table_unset(r->notes, OTEL_REQ_HANDLE_KEY);

    OTEL_SDK_STATUS_CODE res;
    std::unique_ptr<otel::core::ResponsePayload> responsePayload
        (new otel::core::ResponsePayload);
    responsePayload->status_code = r->status;

    if (otel_requestHasErrors(r))
    {
        std::ostringstream oss;
        oss << r->status;
        res = wsAgent.endRequest
            (reqHandle, oss.str().c_str(), responsePayload.get());
    }
    else
    {
        res = wsAgent.endRequest(
            reqHandle, NULL, responsePayload.get());
    }

    if (OTEL_ISSUCCESS(res))
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
// to do with "opentelemetry" stuff.
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

apr_status_t ApacheHooks::otel_child_exit(void* data)
{
    server_rec* s = static_cast<server_rec*>(data);
    const char* sname = (s->server_hostname ? s->server_hostname : "");

    void* retdata;
    apr_pool_userdata_get(&retdata, OTEL_CONFIG_KEY, s->process->pool);
    otel_cfg* our_config = static_cast<otel_cfg*>(retdata);

    if (our_config)
    {
        ApacheTracing::writeError(s, __func__, "terminating agent (sname=%s)", sname);
        wsAgent.term();
    }

    return APR_SUCCESS;
}

void ApacheHooks::otel_child_init(apr_pool_t* p, server_rec* s)
{
    ApacheTracing::logStartupTrace(s);
    apr_pool_cleanup_register(p, s, otel_child_exit, otel_child_exit);
}

// Stage Hooks

int ApacheHooksForStage::otel_hook_header_parser1(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HEADER_PARSER1);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_header_parser2(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HEADER_PARSER2);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_header_parser3(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HEADER_PARSER3);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_header_parser4(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HEADER_PARSER4);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_header_parser5(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HEADER_PARSER5);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_quick_handler1(request_rec* r, int i)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_QUICK_HANDLER1);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_quick_handler2(request_rec* r, int i)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_QUICK_HANDLER2);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_quick_handler3(request_rec* r, int i)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_QUICK_HANDLER3);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_quick_handler4(request_rec* r, int i)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_QUICK_HANDLER4);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_quick_handler5(request_rec* r, int i)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_QUICK_HANDLER5);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_access_checker1(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_ACCESS_CHECKER1);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_access_checker2(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_ACCESS_CHECKER2);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_access_checker3(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_ACCESS_CHECKER3);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_access_checker4(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_ACCESS_CHECKER4);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_access_checker5(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_ACCESS_CHECKER5);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_check_user_id1(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_CHECK_USER_ID1);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_check_user_id2(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_CHECK_USER_ID2);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_check_user_id3(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_CHECK_USER_ID3);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_check_user_id4(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_CHECK_USER_ID4);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_check_user_id5(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_CHECK_USER_ID5);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_auth_checker1(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_AUTH_CHECKER1);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_auth_checker2(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_AUTH_CHECKER2);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_auth_checker3(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_AUTH_CHECKER3);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_auth_checker4(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_AUTH_CHECKER4);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_auth_checker5(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_AUTH_CHECKER5);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_type_checker1(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_TYPE_CHECKER1);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_type_checker2(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_TYPE_CHECKER2);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_type_checker3(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_TYPE_CHECKER3);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_type_checker4(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_TYPE_CHECKER4);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_type_checker5(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_TYPE_CHECKER5);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_fixups1(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_FIXUPS1);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_fixups2(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_FIXUPS2);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_fixups3(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_FIXUPS3);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_fixups4(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_FIXUPS4);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_fixups5(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_FIXUPS5);
    return DECLINED;
}

void ApacheHooksForStage::otel_hook_insert_filter1(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_INSERT_FILTER1);
}

void ApacheHooksForStage::otel_hook_insert_filter2(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_INSERT_FILTER2);
}


void ApacheHooksForStage::otel_hook_insert_filter3(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_INSERT_FILTER3);
}

void ApacheHooksForStage::otel_hook_insert_filter4(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_INSERT_FILTER4);
}

void ApacheHooksForStage::otel_hook_insert_filter5(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_INSERT_FILTER5);
}

// The HANDLER stage does not always run all the hooks.  After a hook returns "OK", the stage
// will end and the remaining hooks are ignored. We use this behavior to ignore any backend calls
// that do not go out to another tier. Thus, if we hit any of the backend_end hook in HANDLER,
// we ignore that backend call in the SDK.
//
// ASSUMPTION: if a module returns DECLINED, it does not go out to another tier.
int ApacheHooksForStage::otel_hook_handler1(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER1,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler2(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER2,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler3(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER3,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler4(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER4,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler5(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER5,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler6(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER6,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler7(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER7,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler8(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER8,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler9(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER9,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler10(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER10,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler11(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER11,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler12(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER12,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler13(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER13,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler14(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER14,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler15(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER15,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler16(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER16,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler17(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER17,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler18(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER18,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler19(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER19,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_handler20(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_HANDLER20,
            true,  // always run
            true); // ignore backend if one exists
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_log_transaction1(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_LOG_TRANSACTION1);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_log_transaction2(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_LOG_TRANSACTION2);
    return DECLINED;
}

int ApacheHooksForStage::otel_hook_log_transaction3(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_LOG_TRANSACTION3);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_log_transaction4(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_LOG_TRANSACTION4);
    return DECLINED;
}
int ApacheHooksForStage::otel_hook_log_transaction5(request_rec* r)
{
    ApacheHooks::otel_startInteraction(
            r,
            HookContainer::OTEL_ENDPOINT_LOG_TRANSACTION5);
    return DECLINED;
}

// These hooks are for stopping interactions after a module
int ApacheHooks::otel_hook_interaction_end(request_rec *r)
{
    otel_stopInteraction(r);
    return DECLINED;
}

// The HANDLER stage does not always run all the hooks.  After a hook returns "OK", the stage
// will end and the remaining hooks are ignored. We use this behavior to ignore any endpoint calls
// that do not go out to another tier. Thus, if we hit any of the interaction_end hook in HANDLER,
// we ignore that endpoint call in the SDK.
//
// ASSUMPTION: if a module returns DECLINED, it does not go out to another tier.
int ApacheHooks::otel_hook_interaction_end_handler(request_rec *r)
{
    otel_stopInteraction(r, true, true); // always run and ignore backend if triggered
    return DECLINED;
}

int ApacheHooks::otel_hook_interaction_end_quick_handler(request_rec *r, int i)
{
    otel_stopInteraction(r);
    return DECLINED;
}

void ApacheHooks::otel_hook_interaction_end_insert_filter(request_rec *r)
{
    // Add our output filter into the chain for this request.
    ap_add_output_filter(OTEL_OUTPUT_FILTER_NAME, NULL, r, r->connection);
    otel_stopInteraction(r);
}

int ApacheHooks::otel_hook_interaction_end_log_transaction(request_rec *r)
{
    otel_stopInteraction(r, true, false); // always run and do not ignore backends
    return DECLINED;
}

template<typename T, typename S>
void ApacheHooksForStage::insertHooksForStage(
        apr_pool_t* p,
        hook_get_t getModules,
        T setHook,
        const std::vector<S> &beginHandlers,
        const std::vector<HookContainer::otel_endpoint_indexes> &indexes,
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

const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::otel_header_parser_hooks =
        {otel_hook_header_parser1
        ,otel_hook_header_parser2
        ,otel_hook_header_parser3
        ,otel_hook_header_parser4
        ,otel_hook_header_parser5};
const std::vector<HookContainer::otel_endpoint_indexes> ApacheHooksForStage::otel_header_parser_indexes =
        {HookContainer::OTEL_ENDPOINT_HEADER_PARSER1
        ,HookContainer::OTEL_ENDPOINT_HEADER_PARSER2
        ,HookContainer::OTEL_ENDPOINT_HEADER_PARSER3
        ,HookContainer::OTEL_ENDPOINT_HEADER_PARSER4
        ,HookContainer::OTEL_ENDPOINT_HEADER_PARSER5};
const std::vector<ApacheHooksForStage::quickHandlerHooks> ApacheHooksForStage::otel_quick_handler_hooks =
        {otel_hook_quick_handler1
        ,otel_hook_quick_handler2
        ,otel_hook_quick_handler3
        ,otel_hook_quick_handler4
        ,otel_hook_quick_handler5};
const std::vector<HookContainer::otel_endpoint_indexes> ApacheHooksForStage::otel_quick_handler_indexes =
        {HookContainer::OTEL_ENDPOINT_QUICK_HANDLER1
        ,HookContainer::OTEL_ENDPOINT_QUICK_HANDLER2
        ,HookContainer::OTEL_ENDPOINT_QUICK_HANDLER3
        ,HookContainer::OTEL_ENDPOINT_QUICK_HANDLER4
        ,HookContainer::OTEL_ENDPOINT_QUICK_HANDLER5};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::otel_access_checker_hooks =
        {otel_hook_access_checker1
        ,otel_hook_access_checker2
        ,otel_hook_access_checker3
        ,otel_hook_access_checker4
        ,otel_hook_access_checker5};
const std::vector<HookContainer::otel_endpoint_indexes> ApacheHooksForStage::otel_access_checker_indexes =
        {HookContainer::OTEL_ENDPOINT_ACCESS_CHECKER1
        ,HookContainer::OTEL_ENDPOINT_ACCESS_CHECKER2
        ,HookContainer::OTEL_ENDPOINT_ACCESS_CHECKER3
        ,HookContainer::OTEL_ENDPOINT_ACCESS_CHECKER4
        ,HookContainer::OTEL_ENDPOINT_ACCESS_CHECKER5};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::otel_check_user_id_hooks =
        {otel_hook_check_user_id1
        ,otel_hook_check_user_id2
        ,otel_hook_check_user_id3
        ,otel_hook_check_user_id4
        ,otel_hook_check_user_id5};
const std::vector<HookContainer::otel_endpoint_indexes> ApacheHooksForStage::otel_check_user_id_indexes =
        {HookContainer::OTEL_ENDPOINT_CHECK_USER_ID1
        ,HookContainer::OTEL_ENDPOINT_CHECK_USER_ID2
        ,HookContainer::OTEL_ENDPOINT_CHECK_USER_ID3
        ,HookContainer::OTEL_ENDPOINT_CHECK_USER_ID4
        ,HookContainer::OTEL_ENDPOINT_CHECK_USER_ID5};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::otel_auth_checker_hooks =
        {otel_hook_auth_checker1
        ,otel_hook_auth_checker2
        ,otel_hook_auth_checker3
        ,otel_hook_auth_checker4
        ,otel_hook_auth_checker5};
const std::vector<HookContainer::otel_endpoint_indexes> ApacheHooksForStage::otel_auth_checker_indexes =
        {HookContainer::OTEL_ENDPOINT_AUTH_CHECKER1
        ,HookContainer::OTEL_ENDPOINT_AUTH_CHECKER2
        ,HookContainer::OTEL_ENDPOINT_AUTH_CHECKER3
        ,HookContainer::OTEL_ENDPOINT_AUTH_CHECKER4
        ,HookContainer::OTEL_ENDPOINT_AUTH_CHECKER5};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::otel_type_checker_hooks =
        {otel_hook_type_checker1
        ,otel_hook_type_checker2
        ,otel_hook_type_checker3
        ,otel_hook_type_checker4
        ,otel_hook_type_checker5};
const std::vector<HookContainer::otel_endpoint_indexes> ApacheHooksForStage::otel_type_checker_indexes =
        {HookContainer::OTEL_ENDPOINT_TYPE_CHECKER1
        ,HookContainer::OTEL_ENDPOINT_TYPE_CHECKER2
        ,HookContainer::OTEL_ENDPOINT_TYPE_CHECKER3
        ,HookContainer::OTEL_ENDPOINT_TYPE_CHECKER4
        ,HookContainer::OTEL_ENDPOINT_TYPE_CHECKER5};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::otel_fixups_hooks =
        {otel_hook_fixups1
        ,otel_hook_fixups2
        ,otel_hook_fixups3
        ,otel_hook_fixups4
        ,otel_hook_fixups5};
const std::vector<HookContainer::otel_endpoint_indexes> ApacheHooksForStage::otel_fixups_indexes =
        {HookContainer::OTEL_ENDPOINT_FIXUPS1
        ,HookContainer::OTEL_ENDPOINT_FIXUPS2
        ,HookContainer::OTEL_ENDPOINT_FIXUPS3
        ,HookContainer::OTEL_ENDPOINT_FIXUPS4
        ,HookContainer::OTEL_ENDPOINT_FIXUPS5};
const std::vector<ApacheHooksForStage::filterHooks> ApacheHooksForStage::otel_insert_filter_hooks =
        {otel_hook_insert_filter1
        ,otel_hook_insert_filter2
        ,otel_hook_insert_filter3
        ,otel_hook_insert_filter4
        ,otel_hook_insert_filter5};
const std::vector<HookContainer::otel_endpoint_indexes> ApacheHooksForStage::otel_insert_filter_indexes =
        {HookContainer::OTEL_ENDPOINT_INSERT_FILTER1
        ,HookContainer::OTEL_ENDPOINT_INSERT_FILTER2
        ,HookContainer::OTEL_ENDPOINT_INSERT_FILTER3
        ,HookContainer::OTEL_ENDPOINT_INSERT_FILTER4
        ,HookContainer::OTEL_ENDPOINT_INSERT_FILTER5};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::otel_handler_hooks =
        {otel_hook_handler1
        ,otel_hook_handler2
        ,otel_hook_handler3
        ,otel_hook_handler4
        ,otel_hook_handler5
        ,otel_hook_handler6
        ,otel_hook_handler7
        ,otel_hook_handler8
        ,otel_hook_handler9
        ,otel_hook_handler10
        ,otel_hook_handler11
        ,otel_hook_handler12
        ,otel_hook_handler13
        ,otel_hook_handler14
        ,otel_hook_handler15
        ,otel_hook_handler16
        ,otel_hook_handler17
        ,otel_hook_handler18
        ,otel_hook_handler19
        ,otel_hook_handler20};
const std::vector<HookContainer::otel_endpoint_indexes> ApacheHooksForStage::otel_handler_indexes =
        {HookContainer::OTEL_ENDPOINT_HANDLER1
        ,HookContainer::OTEL_ENDPOINT_HANDLER2
        ,HookContainer::OTEL_ENDPOINT_HANDLER3
        ,HookContainer::OTEL_ENDPOINT_HANDLER4
        ,HookContainer::OTEL_ENDPOINT_HANDLER5
        ,HookContainer::OTEL_ENDPOINT_HANDLER6
        ,HookContainer::OTEL_ENDPOINT_HANDLER7
        ,HookContainer::OTEL_ENDPOINT_HANDLER8
        ,HookContainer::OTEL_ENDPOINT_HANDLER9
        ,HookContainer::OTEL_ENDPOINT_HANDLER10
        ,HookContainer::OTEL_ENDPOINT_HANDLER11
        ,HookContainer::OTEL_ENDPOINT_HANDLER12
        ,HookContainer::OTEL_ENDPOINT_HANDLER13
        ,HookContainer::OTEL_ENDPOINT_HANDLER14
        ,HookContainer::OTEL_ENDPOINT_HANDLER15
        ,HookContainer::OTEL_ENDPOINT_HANDLER16
        ,HookContainer::OTEL_ENDPOINT_HANDLER17
        ,HookContainer::OTEL_ENDPOINT_HANDLER18
        ,HookContainer::OTEL_ENDPOINT_HANDLER19
        ,HookContainer::OTEL_ENDPOINT_HANDLER20};
const std::vector<ApacheHooksForStage::processRequestHooks> ApacheHooksForStage::otel_log_transaction_hooks =
        {otel_hook_log_transaction1
        ,otel_hook_log_transaction2
        ,otel_hook_log_transaction3
        ,otel_hook_log_transaction4
        ,otel_hook_log_transaction5};
const std::vector<HookContainer::otel_endpoint_indexes> ApacheHooksForStage::otel_log_transaction_indexes =
        {HookContainer::OTEL_ENDPOINT_LOG_TRANSACTION1
        ,HookContainer::OTEL_ENDPOINT_LOG_TRANSACTION2
        ,HookContainer::OTEL_ENDPOINT_LOG_TRANSACTION3
        ,HookContainer::OTEL_ENDPOINT_LOG_TRANSACTION4
        ,HookContainer::OTEL_ENDPOINT_LOG_TRANSACTION5};
