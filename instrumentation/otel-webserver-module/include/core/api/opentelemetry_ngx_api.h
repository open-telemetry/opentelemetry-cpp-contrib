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

#ifndef OPENTELEMETRY_NGX_API_H
#define OPENTELEMETRY_NGX_API_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

const char* httpHeaders[] = {"traceparent", "tracestate"};
const size_t headers_len = sizeof(httpHeaders)/sizeof(httpHeaders[0]);

typedef struct{
    char* name;
    char* value;
}http_headers;

/* Structure for the request payload */
typedef struct {
    const char* uri;
    const char* server_name;
    const char* scheme;
    const char* flavor;
    const char* hostname;
    const char* protocol;
    const char* http_get_param;
    const char* http_post_param;
    const char* request_method;
    const char* client_ip;
    http_headers* propagation_headers;
    http_headers* request_headers;

    int propagation_count;
    int request_headers_count;
}request_payload;

typedef struct {
    http_headers* response_headers;
    int response_headers_count;

    unsigned int status_code;
}response_payload;

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
void populatePayload(request_payload* req_payload, void* payload);
void setRequestResponseHeaders(const char* request, const char* response);
OTEL_SDK_STATUS_CODE opentelemetry_core_init(OTEL_SDK_ENV_RECORD* env, unsigned numberOfRecords, struct cNode *rootCN);
OTEL_SDK_STATUS_CODE startRequest(const char* wscontext, request_payload* req_payload, OTEL_SDK_HANDLE_REQ* reqHandle);
OTEL_SDK_STATUS_CODE startModuleInteraction(OTEL_SDK_HANDLE_REQ req_handle_key, const char* module_name, const char* stage, bool resolveBackends, OTEL_SDK_ENV_RECORD* propagationHeaders, int* ix);
OTEL_SDK_STATUS_CODE stopModuleInteraction(OTEL_SDK_HANDLE_REQ req_handle_key, const char* backendName, const char* backendType, unsigned int err_code, const char* msg);
OTEL_SDK_STATUS_CODE endRequest(OTEL_SDK_HANDLE_REQ req_handle_key, const char* errMsg, response_payload* payload);

#ifdef	__cplusplus
}
#endif
#endif /* OPENTELEMETRY_NGX_API_H ends */
