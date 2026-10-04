#!/bin/bash

TEST_NAME="Check that span status is Error when upstream responds with 5xx"

. tools.sh

setup_test () {

EXTRA_HTTPD_MODS="proxy proxy_http"

cat << EOF > ${HTTPD_CONFIG}
OpenTelemetryExporter   file
OpenTelemetryPath ${OUTPUT_SPANS}

<IfModule mod_proxy.c>
    ProxyPass "/err" "${ENDPOINT_PROXY}/err"
    ProxyPassReverse "/err" "${ENDPOINT_PROXY}/err"
 </IfModule>
EOF

}

proxy () {
   echo -e "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\nContent-Length: 0\r\n\r\n"
}

run_test() {
    HTTP_CODE=`curl --silent -m ${CURL_TIMEOUT} -o /dev/null -w '%{http_code}' ${ENDPOINT_URL}/err`
    [ "${HTTP_CODE}" == "500" ] || failHttpd "Expected HTTP 500 from proxied request but got ${HTTP_CODE}"
}


check_results() {
   echo "Checking that exactly two spans were created (client one and server one)"
   count '{' 2

   echo "Checking that both spans have status Error"
   checkSpanStatus Client Error
   checkSpanStatus Server Error

   count 'http.status_code: 500' 2
}

run $@
