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

#ifndef __PAYLOAD_H
#define __PAYLOAD_H

#include <unordered_map>

namespace appd {
namespace core {


//-----------------------------------------------------------------------------------------
// RequestPayload
// Used for following purposes:
//    1) Get the incoming request information from the web-server and pass it to the appdynamics
//		 core sdk library
//-----------------------------------------------------------------------------------------
class RequestPayload
{
	std::string uri;							/* Request URI of the incoming request */
	std::string request_protocol;				/* Protocol string, as given to us, or HTTP/0.9 */
	std::string http_get_parameter;				/* The QUERY_ARGS extracted from the GET request */
	std::string http_post_parameter;			/* The QUERY_ARGS extracted from the POST request */
	std::string http_request_method;			/* Request method (eg. GET, HEAD, POST, etc.) */

	std::unordered_map<std::string, std::string> http_headers; /* HTTP Request headers: Cookie, Referer, SM_USER*/

public:
	void set_http_headers(const std::string& key, const std::string& value)
    {
        http_headers[key] = value;
    }
    void set_uri(const char* URI) { uri = URI; }
    void set_request_protocol(const char* requestProtocol) {request_protocol = requestProtocol; }
    void set_http_get_parameter(const char* httpGetParameter) {http_get_parameter = httpGetParameter; }
    void set_http_post_parameter(const char* httpPostParameter) {http_post_parameter = httpPostParameter; }
    void set_http_request_method(const char* httpRequestMethod) {http_request_method = httpRequestMethod; }

	std::string get_uri() {	return uri; }
	std::string get_request_protocol() { return request_protocol; }
	std::string get_http_get_parameter() { return http_get_parameter; }
	std::string get_http_post_parameter() { return http_post_parameter; }
	std::string get_http_request_method() { return http_request_method; }
	std::unordered_map<std::string, std::string> get_http_headers() { return http_headers; }
};

struct InteractionPayload
{
	// Endpoint
	std::string moduleName;
	std::string phaseName;
	bool resolveBackends;

    std::string target;
    std::string scheme;
    long port;

	InteractionPayload() {}

	InteractionPayload(std::string module, std::string phase, bool b, std::string aTarget, std::string aScheme, long aPort) 
	: moduleName(module), phaseName(phase), resolveBackends(b), target(aTarget), scheme(aScheme), port(aPort)
	{}
};

struct EndInteractionPayload
{
	std::string backendName;
	std::string backendType;

	long errorCode;
	std::string errorMsg;

	EndInteractionPayload() {}

	EndInteractionPayload(std::string bName, std::string bType, long eCode, std::string eMsg) :
	backendName(bName),
	backendType(bType),
	errorCode(eCode),
	errorMsg(eMsg)
	{}
};

} // appd
} // core

#endif
