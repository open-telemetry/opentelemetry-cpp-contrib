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

#ifndef __PAYLOAD_H
#define __PAYLOAD_H

#include <unordered_map>
#include "sdkwrapper/SdkConstants.h"

namespace otel {
namespace core {

//-----------------------------------------------------------------------------------------
// RequestPayload
// Used for following purposes:
//    1) Get the incoming request information from the web-server and pass it to the otel
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
	std::unordered_map<std::string, std::string> request_headers;

	std::string server_name;
    std::string scheme;
    std::string host;
    std::string target;
    std::string flavor;
    std::string client_ip;
    long port = 80;

public:
	void set_http_headers(const std::string& key, const std::string& value)
    {
        http_headers[key] = value;
    }
    void set_request_headers(const std::string& key, const std::string& value)
    {
    	request_headers[key] = value;
    }
    void set_uri(const char* URI) { uri = URI; }
    void set_request_protocol(const char* requestProtocol) {request_protocol = requestProtocol; }
    void set_http_get_parameter(const char* httpGetParameter) {http_get_parameter = httpGetParameter; }
    void set_http_post_parameter(const char* httpPostParameter) {http_post_parameter = httpPostParameter; }
    void set_http_request_method(const char* httpRequestMethod) {http_request_method = httpRequestMethod; }
    void set_server_name(const char* serverName) {server_name = serverName; }
    void set_scheme(const char* aScheme) {scheme = aScheme; }
    void set_host(const char* aHost) {host = aHost; }
    void set_target(const char* aTarget) {target = aTarget; }
    void set_flavor(const char* aflavor) {flavor = aflavor; }
    void set_client_ip(const char* clientIp) {client_ip = clientIp; }
    void set_port(long aPort) {port = aPort; }


	std::string get_uri() {	return uri; }
	std::string get_request_protocol() { return request_protocol; }
	std::string get_http_get_parameter() { return http_get_parameter; }
	std::string get_http_post_parameter() { return http_post_parameter; }
	std::string get_http_request_method() { return http_request_method; }
	std::unordered_map<std::string, std::string> get_http_headers() { return http_headers; }
	std::string get_server_name() { return server_name; }
    std::string get_scheme() {return scheme; }
    std::string get_host() {return host; }
    std::string get_target() {return target; }
    std::string get_flavor() {return flavor; }
    std::string get_client_ip() {return client_ip; }
    long get_port() {return port; }
    std::unordered_map<std::string, std::string>& get_request_headers() {
    	return request_headers;
    }
};

struct ResponsePayload
{
	std::unordered_map<std::string, std::string> response_headers;
	unsigned int status_code{sdkwrapper::kStatusCodeInit};
};

struct InteractionPayload
{
	// Endpoint
	std::string moduleName;
	std::string phaseName;
	bool resolveBackends;

	InteractionPayload() {}

	InteractionPayload(std::string module, std::string phase, bool b) : moduleName(module), phaseName(phase), resolveBackends(b)
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

} // otel
} // core

#endif
