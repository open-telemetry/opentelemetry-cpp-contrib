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
#pragma once

namespace otel {
namespace core {
namespace sdkwrapper {

const std::string kServiceName = "service.name";
const std::string kServiceNamespace = "service.namespace";
const std::string kServiceInstanceId = "service.instance.id";
const std::string kOtelWebEngineName = "webengine.name";
const std::string kOtelWebEngineVersion = "webengine.version";
const std::string kOtelWebEngineDescription = "webengine.description";
const std::string kHttpErrorCode = "HTTP ERROR CODE:";
const std::string kAttrHTTPServerName         = "http.server_name";
const std::string kAttrHTTPMethod             = "http.method";
const std::string kAttrHTTPScheme             = "http.scheme";
const std::string kAttrNetHostName            = "net.host.name";
const std::string kAttrHTTPTarget             = "http.target";
const std::string kAttrHTTPUrl                = "http.url";
const std::string kAttrHTTPFlavor             = "http.flavor";
const std::string kAttrHTTPClientIP           = "http.client_ip";
const std::string kAttrHTTPStatusCode         = "http.status_code";
const std::string kAttrNETHostPort            = "net.host.port";
const std::string kAttrRequestProtocol        = "request_protocol";
const std::string kHTTPFlavor1_0                 = "1.0";
const std::string kHTTPFlavor1_1                = "1.1";


constexpr int HTTP_ERROR_1XX = 100;
constexpr int HTTP_ERROR_4XX = 400;
constexpr int HTTP_ERROR_5XX = 500;
constexpr int HTTP_PROTO_1000 = 1000;
constexpr int HTTP_PROTO_1001 = 1001;

constexpr unsigned int kStatusCodeInit = 0;

} // sdkwrapper
} // core
} // otel
