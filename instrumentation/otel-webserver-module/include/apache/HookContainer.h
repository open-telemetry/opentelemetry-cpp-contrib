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

#ifndef HOOKSCONTAINER_H
#define HOOKSCONTAINER_H

#include <map>
#include <string>
#include "httpd.h"

// store meta data for each apache hook. This also creates a moduleList (array of char*) used for
// the hook successor and predecessor.
class HookInfo
{
public:
    HookInfo(const std::string& stage, const std::string& module, int i);

    const char* moduleList[2];
    std::string stage;
    std::string module;
    int order;
};

class HookContainer
{
public:
    // The following are indentifiers into the hookContainer storage. This is needed to
    // store and retrieve the endpoint module name.
    enum otel_endpoint_indexes
    {
        OTEL_ENDPOINT_HEADER_PARSER1
        ,OTEL_ENDPOINT_HEADER_PARSER2
        ,OTEL_ENDPOINT_HEADER_PARSER3
        ,OTEL_ENDPOINT_HEADER_PARSER4
        ,OTEL_ENDPOINT_HEADER_PARSER5
        ,OTEL_ENDPOINT_QUICK_HANDLER1
        ,OTEL_ENDPOINT_QUICK_HANDLER2
        ,OTEL_ENDPOINT_QUICK_HANDLER3
        ,OTEL_ENDPOINT_QUICK_HANDLER4
        ,OTEL_ENDPOINT_QUICK_HANDLER5
        ,OTEL_ENDPOINT_ACCESS_CHECKER1
        ,OTEL_ENDPOINT_ACCESS_CHECKER2
        ,OTEL_ENDPOINT_ACCESS_CHECKER3
        ,OTEL_ENDPOINT_ACCESS_CHECKER4
        ,OTEL_ENDPOINT_ACCESS_CHECKER5
        ,OTEL_ENDPOINT_CHECK_USER_ID1
        ,OTEL_ENDPOINT_CHECK_USER_ID2
        ,OTEL_ENDPOINT_CHECK_USER_ID3
        ,OTEL_ENDPOINT_CHECK_USER_ID4
        ,OTEL_ENDPOINT_CHECK_USER_ID5
        ,OTEL_ENDPOINT_AUTH_CHECKER1
        ,OTEL_ENDPOINT_AUTH_CHECKER2
        ,OTEL_ENDPOINT_AUTH_CHECKER3
        ,OTEL_ENDPOINT_AUTH_CHECKER4
        ,OTEL_ENDPOINT_AUTH_CHECKER5
        ,OTEL_ENDPOINT_TYPE_CHECKER1
        ,OTEL_ENDPOINT_TYPE_CHECKER2
        ,OTEL_ENDPOINT_TYPE_CHECKER3
        ,OTEL_ENDPOINT_TYPE_CHECKER4
        ,OTEL_ENDPOINT_TYPE_CHECKER5
        ,OTEL_ENDPOINT_FIXUPS1
        ,OTEL_ENDPOINT_FIXUPS2
        ,OTEL_ENDPOINT_FIXUPS3
        ,OTEL_ENDPOINT_FIXUPS4
        ,OTEL_ENDPOINT_FIXUPS5
        ,OTEL_ENDPOINT_INSERT_FILTER1
        ,OTEL_ENDPOINT_INSERT_FILTER2
        ,OTEL_ENDPOINT_INSERT_FILTER3
        ,OTEL_ENDPOINT_INSERT_FILTER4
        ,OTEL_ENDPOINT_INSERT_FILTER5
        ,OTEL_ENDPOINT_HANDLER1
        ,OTEL_ENDPOINT_HANDLER2
        ,OTEL_ENDPOINT_HANDLER3
        ,OTEL_ENDPOINT_HANDLER4
        ,OTEL_ENDPOINT_HANDLER5
        ,OTEL_ENDPOINT_HANDLER6
        ,OTEL_ENDPOINT_HANDLER7
        ,OTEL_ENDPOINT_HANDLER8
        ,OTEL_ENDPOINT_HANDLER9
        ,OTEL_ENDPOINT_HANDLER10
        ,OTEL_ENDPOINT_HANDLER11
        ,OTEL_ENDPOINT_HANDLER12
        ,OTEL_ENDPOINT_HANDLER13
        ,OTEL_ENDPOINT_HANDLER14
        ,OTEL_ENDPOINT_HANDLER15
        ,OTEL_ENDPOINT_HANDLER16
        ,OTEL_ENDPOINT_HANDLER17
        ,OTEL_ENDPOINT_HANDLER18
        ,OTEL_ENDPOINT_HANDLER19
        ,OTEL_ENDPOINT_HANDLER20
        ,OTEL_ENDPOINT_LOG_TRANSACTION1
        ,OTEL_ENDPOINT_LOG_TRANSACTION2
        ,OTEL_ENDPOINT_LOG_TRANSACTION3
        ,OTEL_ENDPOINT_LOG_TRANSACTION4
        ,OTEL_ENDPOINT_LOG_TRANSACTION5

        ,OTEL_MAX_ENDPOINTS
    };

    static HookContainer& getInstance();
    void addHook(otel_endpoint_indexes index, const std::string& stage, const std::string& module, int order);
    const std::string& getStage(otel_endpoint_indexes index) const;
    const std::string& getModule(otel_endpoint_indexes index) const;
    const char* const* getModuleList(otel_endpoint_indexes index) const;
    void traceHooks(request_rec* r) const;

private:
    HookContainer() {}
    std::map<int, HookInfo> hooks;
};

#endif
