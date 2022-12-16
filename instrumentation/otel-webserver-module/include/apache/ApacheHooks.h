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

// This file includes are apache module hook functions
#ifndef APACHEHOOKS_H
#define APACHEHOOKS_H

#include <api/OpentelemetrySdk.h>
#include <unordered_map>
#include "HookContainer.h"
#include "ExcludedModules.h"

class ApacheHooks
{
public:
    static std::string m_aggregatorCommDir;
    static bool m_reportAllStages;
    static const char* OTEL_CONFIG_KEY;
    static const char* OTEL_CORRELATION_HEADER_KEY;
    static const char* OTEL_INTERACTION_HANDLE_KEY;
    static const char* OTEL_REQ_HANDLE_KEY;
    static const int LOWEST_HTTP_ERROR_CODE = 400;
    static const int CONFIG_COUNT = 17; // Number of key value pairs in config
    static const std::initializer_list<const char*> httpHeaders;
    static void registerHooks(apr_pool_t *p);

    friend class ApacheHooksForStage;

private:
    static void otel_child_init(apr_pool_t *p, server_rec *s);
    static apr_status_t otel_child_exit(void* data);

    static bool initialize_opentelemetry(const request_rec *r);
    static int otel_hook_header_parser_begin(request_rec *r);
    static int otel_hook_log_transaction_end(request_rec *r);

    // Callback to start an Interaction

    // The HANDLER stage does not always run all the hooks.  After a hook returns "OK", the stage
    // will end and the remaining hooks are ignored. We use this behavior to ignore any backend calls
    // that do not go out to another tier. Thus, if we hit any of the backend_end hook in HANDLER,
    // we ignore that backend call in the SDK.
    //
    // ASSUMPTION: if a module returns DECLINED, it does not go out to another tier.
    static OTEL_SDK_STATUS_CODE otel_startInteraction(
            request_rec *r,
            HookContainer::otel_endpoint_indexes,
            bool isAlwaysRunStage = false,
            bool ignoreBackend = false);

    // Callback to stop an Interaction
    static void otel_stopInteraction(request_rec *r, bool isAlwaysRunStage = false, bool ignoreBackend = false);

    static void otel_payload_decorator(request_rec* r, std::unordered_map<std::string, std::string> propagationHeaders);
    static bool otel_requestHasErrors(request_rec* r);
    static apr_status_t otel_output_filter(ap_filter_t* f, apr_bucket_brigade* pbb);

    // These hooks are for stopping interaction after a module callback in hook stages
    static int otel_hook_interaction_end(request_rec *r);
    static int otel_hook_interaction_end_handler(request_rec *r); // ignore backends in handler stage
    static int otel_hook_interaction_end_quick_handler(request_rec *r, int i);
    static void otel_hook_interaction_end_insert_filter(request_rec *r);
    static int otel_hook_interaction_end_log_transaction(request_rec *r);
};

class ApacheHooksForStage
{
public:
    // For reference: Stages and their hook function prototypes, we can hook into the modules
    // for the following stages
    //
    // int otel_hook_create_request(request_rec *r);
    // void otel_hook_pre_read_request(request_rec *r, conn_rec* c);
    // int otel_hook_post_read_request(request_rec *r);
    // int otel_hook_header_parser(request_rec *r);
    // const char* otel_hook_http_scheme(const request_rec *r);
    // apr_port_t otel_hook_default_port(const request_rec *r);
    // int otel_hook_quick_handler(request_rec *r, int i);
    // int otel_hook_translate_name(request_rec *r);
    // int otel_hook_map_to_storage(request_rec *r);
    // int otel_hook_access_checker_ex(request_rec *r);
    // int otel_hook_access_checker(request_rec *r);
    // int otel_hook_check_user_id(request_rec *r);
    // int otel_hook_note_auth_failure(request_rec *r, const char* c);
    // int otel_hook_auth_checker(request_rec *r);
    // int otel_hook_type_checker(request_rec *r);
    // int otel_hook_fixups(request_rec *r);
    // void otel_hook_insert_filter(request_rec *r);
    // int otel_hook_handler(request_rec *r);
    // int otel_hook_log_transaction(request_rec *r);
    // void otel_hook_insert_error_filter(request_rec *r);
    // int otel_hook_generate_log_id(const conn_rec *c, const request_rec *r, const char**);

    /*
        function prototypes for the apache module hook callbacks according to different stages
    */
    typedef int         (*processRequestHooks) (request_rec*); // create_request, post_read_request, header_parser,
            // translate_name, map_to_storage, access_checker_ex, access_checker, check_user_id, auth_checker,
            // type_checker, fixups, handler, log_transaction
    typedef int         (*quickHandlerHooks) (request_rec*, int); // quick_handler
    typedef void        (*filterHooks) (request_rec*); // insert_filter, insert_error_filter

    /*
        Hooks to occur before a module to start an interaction for a stage.
        These hooks are stage specific and each hook corresponds to a particular module.
        The number of hooks are proportional to the number of modules instrumented for a stage.
        TODO: Decide among the following stages at what all we need the modules to be instrumented,
              and define the hooks handlers for the same to start an interaction before module callback.
    */
    static int otel_hook_header_parser1(request_rec* r);
    static int otel_hook_header_parser2(request_rec* r);
    static int otel_hook_header_parser3(request_rec* r);
    static int otel_hook_header_parser4(request_rec* r);
    static int otel_hook_header_parser5(request_rec* r);
    static int otel_hook_quick_handler1(request_rec* r, int i);
    static int otel_hook_quick_handler2(request_rec* r, int i);
    static int otel_hook_quick_handler3(request_rec* r, int i);
    static int otel_hook_quick_handler4(request_rec* r, int i);
    static int otel_hook_quick_handler5(request_rec* r, int i);
    static int otel_hook_access_checker1(request_rec* r);
    static int otel_hook_access_checker2(request_rec* r);
    static int otel_hook_access_checker3(request_rec* r);
    static int otel_hook_access_checker4(request_rec* r);
    static int otel_hook_access_checker5(request_rec* r);
    static int otel_hook_check_user_id1(request_rec* r);
    static int otel_hook_check_user_id2(request_rec* r);
    static int otel_hook_check_user_id3(request_rec* r);
    static int otel_hook_check_user_id4(request_rec* r);
    static int otel_hook_check_user_id5(request_rec* r);
    static int otel_hook_auth_checker1(request_rec* r);
    static int otel_hook_auth_checker2(request_rec* r);
    static int otel_hook_auth_checker3(request_rec* r);
    static int otel_hook_auth_checker4(request_rec* r);
    static int otel_hook_auth_checker5(request_rec* r);
    static int otel_hook_type_checker1(request_rec* r);
    static int otel_hook_type_checker2(request_rec* r);
    static int otel_hook_type_checker3(request_rec* r);
    static int otel_hook_type_checker4(request_rec* r);
    static int otel_hook_type_checker5(request_rec* r);
    static int otel_hook_fixups1(request_rec* r);
    static int otel_hook_fixups2(request_rec* r);
    static int otel_hook_fixups3(request_rec* r);
    static int otel_hook_fixups4(request_rec* r);
    static int otel_hook_fixups5(request_rec* r);
    static void otel_hook_insert_filter1(request_rec* r);
    static void otel_hook_insert_filter2(request_rec* r);
    static void otel_hook_insert_filter3(request_rec* r);
    static void otel_hook_insert_filter4(request_rec* r);
    static void otel_hook_insert_filter5(request_rec* r);
    static int otel_hook_handler1(request_rec* r);
    static int otel_hook_handler2(request_rec* r);
    static int otel_hook_handler3(request_rec* r);
    static int otel_hook_handler4(request_rec* r);
    static int otel_hook_handler5(request_rec* r);
    static int otel_hook_handler6(request_rec* r);
    static int otel_hook_handler7(request_rec* r);
    static int otel_hook_handler8(request_rec* r);
    static int otel_hook_handler9(request_rec* r);
    static int otel_hook_handler10(request_rec* r);
    static int otel_hook_handler11(request_rec* r);
    static int otel_hook_handler12(request_rec* r);
    static int otel_hook_handler13(request_rec* r);
    static int otel_hook_handler14(request_rec* r);
    static int otel_hook_handler15(request_rec* r);
    static int otel_hook_handler16(request_rec* r);
    static int otel_hook_handler17(request_rec* r);
    static int otel_hook_handler18(request_rec* r);
    static int otel_hook_handler19(request_rec* r);
    static int otel_hook_handler20(request_rec* r);
    static int otel_hook_log_transaction1(request_rec* r);
    static int otel_hook_log_transaction2(request_rec* r);
    static int otel_hook_log_transaction3(request_rec* r);
    static int otel_hook_log_transaction4(request_rec* r);
    static int otel_hook_log_transaction5(request_rec* r);

    static const std::vector<processRequestHooks> otel_header_parser_hooks;
    static const std::vector<HookContainer::otel_endpoint_indexes> otel_header_parser_indexes;
    static const std::vector<quickHandlerHooks> otel_quick_handler_hooks;
    static const std::vector<HookContainer::otel_endpoint_indexes> otel_quick_handler_indexes;
    static const std::vector<processRequestHooks> otel_access_checker_hooks;
    static const std::vector<HookContainer::otel_endpoint_indexes> otel_access_checker_indexes;
    static const std::vector<processRequestHooks> otel_check_user_id_hooks;
    static const std::vector<HookContainer::otel_endpoint_indexes> otel_check_user_id_indexes;
    static const std::vector<processRequestHooks> otel_auth_checker_hooks;
    static const std::vector<HookContainer::otel_endpoint_indexes> otel_auth_checker_indexes;
    static const std::vector<processRequestHooks> otel_type_checker_hooks;
    static const std::vector<HookContainer::otel_endpoint_indexes> otel_type_checker_indexes;
    static const std::vector<processRequestHooks> otel_fixups_hooks;
    static const std::vector<HookContainer::otel_endpoint_indexes> otel_fixups_indexes;
    static const std::vector<filterHooks> otel_insert_filter_hooks;
    static const std::vector<HookContainer::otel_endpoint_indexes> otel_insert_filter_indexes;
    static const std::vector<processRequestHooks> otel_handler_hooks;
    static const std::vector<HookContainer::otel_endpoint_indexes> otel_handler_indexes;
    static const std::vector<processRequestHooks> otel_log_transaction_hooks;
    static const std::vector<HookContainer::otel_endpoint_indexes> otel_log_transaction_indexes;

    template<typename T, typename S>
    static void insertHooksForStage(
            apr_pool_t* p,
            hook_get_t getModules,
            T setHook,
            const std::vector<S> &beginHandlers,
            const std::vector<HookContainer::otel_endpoint_indexes> &indexes,
            S endHandler,
            const std::string& stage);
};
#endif
