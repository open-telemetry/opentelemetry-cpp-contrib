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
#include "api/OpentelemetrySdk.h"
#include "api/opentelemetry_ngx_api.h"
#include "api/WSAgent.h"
#include "api/Payload.h"
#include <cstring>
#include <sstream>
#include <unordered_set>
#include <algorithm>

otel::core::WSAgent wsAgent; // global variable for interface between Hooks and Core Logic
std::unordered_set<std::string> requestHeadersToCapture;
std::unordered_set<std::string> responseHeadersToCapture;
constexpr char delimiter = ',';

void populatePayload(request_payload* req_payload, void* load)
{
    otel::core::RequestPayload* payload = (otel::core::RequestPayload*)load;
    payload->set_uri(req_payload->uri);
    payload->set_scheme(req_payload->scheme);
    payload->set_flavor(req_payload->flavor);
    payload->set_target(req_payload->uri);
    payload->set_host(req_payload->hostname);
    payload->set_server_name(req_payload->server_name);
    payload->set_request_protocol(req_payload->protocol);
    payload->set_http_post_parameter(req_payload->http_post_param);
    payload->set_http_get_parameter(req_payload->http_get_param);
    payload->set_http_request_method(req_payload->request_method);
    payload->set_client_ip(req_payload->client_ip);

    for(int i=0; i<req_payload->propagation_count; i++){
        payload->set_http_headers(req_payload->propagation_headers[i].name, req_payload->propagation_headers[i].value);
    }

    for (int i = 0; i < req_payload->request_headers_count; i++) {
        std::string key(req_payload->request_headers[i].name);
        if (requestHeadersToCapture.find(key)
            != requestHeadersToCapture.end()) {
            payload->set_request_headers(key,
                req_payload->request_headers[i].value);
        }
    }
}

void setRequestResponseHeaders(const char* request, const char* response)
{
    std::string token;
    std::stringstream ss;

    ss.str(std::string(request));
    while(getline(ss, token, delimiter)) {
        requestHeadersToCapture.insert(token);
    }

    token.clear();
    ss.clear();
    ss.str(std::string(response));
    while(getline(ss, token, delimiter)) {
        responseHeadersToCapture.insert(token);
    }
}

void initDependency()
{
    wsAgent.initDependency();
}

OTEL_SDK_STATUS_CODE opentelemetry_core_init(OTEL_SDK_ENV_RECORD* env, unsigned numberOfRecords, struct cNode *rootCN)
{
    OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;
    struct cNode *curCN = rootCN;

    while(curCN){
        otel::core::WSContextConfig cfg;
        cfg.serviceNamespace = (curCN->cInfo).sNamespace;
        cfg.serviceName = (curCN->cInfo).sName;
        cfg.serviceInstanceId = (curCN->cInfo).sInstanceId;
        wsAgent.addWSContextToCore((curCN->cInfo).cName, &cfg);
        curCN = curCN->next;
    }

    res = wsAgent.init(env, numberOfRecords);
  
    return res;
}

OTEL_SDK_STATUS_CODE startRequest(const char* wscontext, request_payload* req_payload, OTEL_SDK_HANDLE_REQ* reqHandle)
{
    OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;

    std::unique_ptr<otel::core::RequestPayload> requestPayload(new otel::core::RequestPayload);
    populatePayload(req_payload, requestPayload.get());
    res = wsAgent.startRequest(wscontext, requestPayload.get(), reqHandle);

    return res;
}

OTEL_SDK_STATUS_CODE endRequest(OTEL_SDK_HANDLE_REQ req_handle_key, const char* errMsg,
    response_payload* payload)
{
    OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;

    std::unique_ptr<otel::core::ResponsePayload>
        responsePayload(new otel::core::ResponsePayload);
    if (payload != NULL) {
        for (int i = 0; i < payload->response_headers_count; i++) {
            std::string key(payload->response_headers[i].name);
            if (responseHeadersToCapture.find(key)
            != responseHeadersToCapture.end()) {
                responsePayload->response_headers[key]
                    = payload->response_headers[i].value;
            }
        }
        responsePayload->status_code = payload->status_code;
    }

    res = wsAgent.endRequest(req_handle_key, errMsg, responsePayload.get());

    return res;
}

OTEL_SDK_STATUS_CODE startModuleInteraction(OTEL_SDK_HANDLE_REQ req_handle_key, const char* module_name, const char* stage, bool resolveBackends, OTEL_SDK_ENV_RECORD* propagationHeaders, int *ix)
{
    OTEL_SDK_STATUS_CODE res = OTEL_SUCCESS;
    std::unordered_map<std::string, std::string> pHeaders;
    std::string module(module_name);
    std::string m_stage(stage);

    std::unique_ptr<otel::core::InteractionPayload> payload(new otel::core::InteractionPayload(module, m_stage, resolveBackends));
    res = wsAgent.startInteraction(req_handle_key, payload.get(), pHeaders);

    if (OTEL_ISSUCCESS(res))
    {
        if (!pHeaders.empty())
        {
            for (auto itr = pHeaders.begin(); itr != pHeaders.end(); itr++)
            {
                char *temp_key = (char*)malloc(itr->first.size() + 1); 
                std::strcpy(temp_key, itr->first.c_str());
                propagationHeaders[*ix].name = temp_key;
                char *temp_value= (char*)malloc(itr->second.size() + 1); 
                std::strcpy(temp_value, itr->second.c_str());
                 propagationHeaders[*ix].value = temp_value;
                ++(*ix);
            }
        }
    }
    return res;
}

OTEL_SDK_STATUS_CODE stopModuleInteraction(OTEL_SDK_HANDLE_REQ req_handle_key, const char* backendName, const char* backendType, unsigned int err_code, const char* msg)
{
    std::unique_ptr<otel::core::EndInteractionPayload> payload(new otel::core::EndInteractionPayload(backendName, backendType, err_code, msg));
    OTEL_SDK_STATUS_CODE res = wsAgent.endInteraction(req_handle_key, false, payload.get());

    return res;
}
