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

#ifndef OPENTELEMETRY_NGX_API_H
#define OPENTELEMETRY_NGX_API_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#define BAGGAGE "baggage"
#define TRACEPARENT "traceparent"
#define TRACESTATE "tracestate"

const char* httpHeaders[] = {"baggage", "traceparent", "tracestate"};

typedef struct{
    char* name;
    char* value;
}http_headers;

/* Structure for the request payload */
typedef struct {
    const char* uri;
    const char* protocol;
    const char* http_get_param;
    const char* http_post_param;
    const char* request_method;
    http_headers* headers;
}request_payload;

typedef struct{
    const char* cName;
    const char* sNamespace;
    const char* sName;
    const char* sInstanceId;
}contextInfo;

struct cNode{
    contextInfo cInfo;
    struct cNode* next;
};

void initDependency();
void populatePayload(request_payload* req_payload, void* payload, int count);
APPD_SDK_STATUS_CODE opentelemetry_core_init(APPD_SDK_ENV_RECORD* env, unsigned numberOfRecords, struct cNode *rootCN);
APPD_SDK_STATUS_CODE startRequest(const char* wscontext, request_payload* req_payload, APPD_SDK_HANDLE_REQ* reqHandle, int count);
APPD_SDK_STATUS_CODE startModuleInteraction(const char* req_handle_key, const char* module_name, const char* stage, bool resolveBackends, APPD_SDK_ENV_RECORD* propagationHeaders, int* ix, const char* target, const char* scheme, const char* host);
APPD_SDK_STATUS_CODE stopModuleInteraction(const char* req_handle_key, const char* backendName, const char* backendType, unsigned int err_code, const char* msg);
APPD_SDK_STATUS_CODE endRequest(APPD_SDK_HANDLE_REQ req_handle_key, const char* errMsg);

#ifdef	__cplusplus
}
#endif
#endif /* OPENTELEMETRY_NGX_API_H ends */
