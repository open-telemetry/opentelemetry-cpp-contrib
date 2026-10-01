#!/bin/bash

TEST_NAME="Check that 4xx sets Error on client span but leaves server span Unset"

. tools.sh

setup_test () {

EXTRA_HTTPD_MODS="proxy proxy_http"

cat << EOF > ${HTTPD_CONFIG}
OpenTelemetryExporter   file
OpenTelemetryPath ${OUTPUT_SPANS}

<IfModule mod_proxy.c>
    ProxyPass "/missing" "${ENDPOINT_PROXY}/missing"
    ProxyPassReverse "/missing" "${ENDPOINT_PROXY}/missing"
 </IfModule>
EOF

}

proxy () {
   echo -e "HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n"
}

run_test() {
    HTTP_CODE=`curl --silent -m ${CURL_TIMEOUT} -o /dev/null -w '%{http_code}' ${ENDPOINT_URL}/missing`
    [ "${HTTP_CODE}" == "404" ] || failHttpd "Expected HTTP 404 from proxied request but got ${HTTP_CODE}"
}


check_results() {
   echo "Checking that exactly two spans were created (client one and server one)"
   count '{' 2

   checkSpanStatus Client Error
   checkSpanStatus Server Unset

   count 'http.status_code: 404' 2
}

run $@
