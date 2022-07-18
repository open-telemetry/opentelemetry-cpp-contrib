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
#include "api/AppdynamicsSdk.h"
#include "api/opentelemetry_ngx_api.h"
#include "api/WSAgent.h"
#include "api/Payload.h"
#include <cstring>


appd::core::WSAgent wsAgent; // global variable for interface between Hooks and Core Logic

void populatePayload(request_payload* req_payload, void* load, int count)
{
    appd::core::RequestPayload* payload = (appd::core::RequestPayload*)load;
    payload->set_uri(req_payload->uri);
    payload->set_request_protocol(req_payload->protocol);
    payload->set_http_post_parameter(req_payload->http_post_param);
    payload->set_http_get_parameter(req_payload->http_get_param);
    payload->set_http_request_method(req_payload->request_method);

    for(int i=0; i<count; i++){
        payload->set_http_headers(req_payload->headers[i].name, req_payload->headers[i].value);
    }
}

void initDependency()
{
    wsAgent.initDependency();
}

APPD_SDK_STATUS_CODE opentelemetry_core_init(APPD_SDK_ENV_RECORD* env, unsigned numberOfRecords, struct cNode *rootCN)
{
    APPD_SDK_STATUS_CODE res = APPD_SUCCESS;
    struct cNode *curCN = rootCN;

    while(curCN){
        appd::core::WSContextConfig cfg;
        cfg.serviceNamespace = (curCN->cInfo).sNamespace;
        cfg.serviceName = (curCN->cInfo).sName;
        cfg.serviceInstanceId = (curCN->cInfo).sInstanceId;
        wsAgent.addWSContextToCore((curCN->cInfo).cName, &cfg);
        curCN = curCN->next;
    }

    res = wsAgent.init(env, numberOfRecords);
  
    return res;
}

APPD_SDK_STATUS_CODE startRequest(const char* wscontext, request_payload* req_payload, APPD_SDK_HANDLE_REQ* reqHandle, int count)
{
    APPD_SDK_STATUS_CODE res = APPD_SUCCESS;

    std::unique_ptr<appd::core::RequestPayload> requestPayload(new appd::core::RequestPayload);
    populatePayload(req_payload, requestPayload.get(), count);
    res = wsAgent.startRequest(wscontext, requestPayload.get(), reqHandle);

    return res;
}

APPD_SDK_STATUS_CODE endRequest(APPD_SDK_HANDLE_REQ req_handle_key, const char* errMsg)
{
    APPD_SDK_STATUS_CODE res = APPD_SUCCESS;
    res = wsAgent.endRequest(req_handle_key, errMsg);

    return res;
}

APPD_SDK_STATUS_CODE startModuleInteraction(APPD_SDK_HANDLE_REQ req_handle_key, const char* module_name, const char* stage, bool resolveBackends, APPD_SDK_ENV_RECORD* propagationHeaders, int *ix)
{
    APPD_SDK_STATUS_CODE res = APPD_SUCCESS;
    std::unordered_map<std::string, std::string> pHeaders;
    std::string module(module_name);
    std::string m_stage(stage);

    std::unique_ptr<appd::core::InteractionPayload> payload(new appd::core::InteractionPayload(module, m_stage, resolveBackends)); 
    res = wsAgent.startInteraction(req_handle_key, payload.get(), pHeaders);

    if (APPD_ISSUCCESS(res))
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

APPD_SDK_STATUS_CODE stopModuleInteraction(APPD_SDK_HANDLE_REQ req_handle_key, const char* backendName, const char* backendType, unsigned int err_code, const char* msg)
{
    std::unique_ptr<appd::core::EndInteractionPayload> payload(new appd::core::EndInteractionPayload(backendName, backendType, err_code, msg));
    APPD_SDK_STATUS_CODE res = wsAgent.endInteraction(req_handle_key, false, payload.get());

    return res;
}
