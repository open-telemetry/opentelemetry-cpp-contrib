#!/bin/bash

TEST_NAME="Check that outbound span is created when forwarding request with mod_proxy"

. tools.sh

setup_test () {

EXTRA_HTTPD_MODS="proxy proxy_http"

cat << EOF > ${HTTPD_CONFIG}
OpenTelemetryExporter   file
OpenTelemetryPath ${OUTPUT_SPANS}
OpenTelemetryPropagators trace-context
OpenTelemetryIgnoreInbound Off

<IfModule mod_proxy.c>
    ProxyPass "/bar" "${ENDPOINT_PROXY}/hello-world"
    ProxyPassReverse "/bar" "${ENDPOINT_PROXY}/hello-world"
 </IfModule>
EOF

}

TRACE_VER="00"
TRACE_ID="ffeeddccbbaa99887766554433221100"
TRACE_PARENT="0102030405060708"
TRACE_FLAGS="01"

TRACE_STATE="foo=bar"

HEADER_1="traceparent: ${TRACE_VER}-${TRACE_ID}-${TRACE_PARENT}-${TRACE_FLAGS}"
HEADER_2="tracestate: ${TRACE_STATE}"

run_test() {
    ${CURL_CMD} -H "${HEADER_1}" ${ENDPOINT_URL} || fail "Unable to download main page"
}

# this is what will be returned via proxy (this is called from netcat)
proxy () {
   STATUS="200 OK"

   echo "---${REQ_METHOD}--- ${REQ_URL} "`date '+%H:%M:%S (%s)'` > /dev/tty
   env | grep -i hdr > /dev/tty
   echo '---------' > /dev/tty

   # extract everything before dash
   RECV_STATE=${HDR_TRACESTATE%%-*}

   if [ "${HDR_TRACESTATE}" != "${TRACE_STATE}" ]; then
      echo "Wrong header for tracer test" > /dev/tty
      STATUS="401 Test failed"
   fi

   echo -e "HTTP/1.1 ${STATUS}\r\nConnection: close\r\n\r\n"

   date
   sleep 1
   echo "traceparent Header was: ${HDR_TRACEPARENT}"
   echo "tracestate Header was: ${HDR_TRACESTATE}"
   date
}

run_test() {
    curl --fail -v -H "${HEADER_1}" -H "${HEADER_2}" ${ENDPOINT_URL}/bar || failHttpd "Unable to download page with proxy enabled"
}


check_results() {
   echo "Checking that exactly two spans were created (client one and server one)"
   count '{' 2 # total two spans = one incoming and one outgoing

   echo "Checking that trace_id was properly parsed"
   check 'trace_id' ${TRACE_ID}

   # there should be at least one span type client
   grep "span kind     : Client" ${OUTPUT_SPANS} || fail "Span kind client not found"
   # there should be at least one span created (server one) with set parent_span_id as what we've sent
   grep "parent_span_id: ${TRACE_PARENT}" ${OUTPUT_SPANS} || fail "Inbound propagation failed - not found parent span_id ${TRACE_PARENT}"
   # now exclude it and find remaning one
   PROXY_SPAN_ID=`grep -v "parent_span_id: ${TRACE_PARENT}" ${OUTPUT_SPANS} | grep "parent_span_id:" | cut -d ':' -f 2 || fail "Inbound propagation failed - not found parent span_id"`
   # remaining parent_span_id should be span_id of the server one
   grep "span_id       :${PROXY_SPAN_ID}" ${OUTPUT_SPANS} || fail "Outbound propagation failed - it has not set proper parent span"
}

run $@
