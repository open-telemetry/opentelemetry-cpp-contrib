#!/bin/bash

TEST_NAME="Check that outbound span is created when forwarding request with mod_proxy (b3 single line header)"

. tools.sh

setup_test () {

EXTRA_HTTPD_MODS="proxy proxy_http"

cat << EOF > ${HTTPD_CONFIG}
OpenTelemetryExporter   file
OpenTelemetryPath ${OUTPUT_SPANS}
OpenTelemetryPropagators b3
OpenTelemetryIgnoreInbound Off

<IfModule mod_proxy.c>
    ProxyPass "/bar" "${ENDPOINT_PROXY}/hello-world"
    ProxyPassReverse "/bar" "${ENDPOINT_PROXY}/hello-world"
 </IfModule>
EOF

}

TRACE_ID="0abcdefabcdefabcdefabcdefabcdef0"
TRACE_SPAN="0123456789abcdef"
TRACE_FLAGS="1"

HEADER_VAL="${TRACE_ID}-${TRACE_SPAN}-${TRACE_FLAGS}"
HEADER="b3: ${HEADER_VAL}"

# this is what will be returned via proxy (this is called from netcat)
proxy () {
   STATUS="200 OK"

   echo "---${REQ_METHOD}--- ${REQ_URL} "`date '+%H:%M:%S (%s)'` > /dev/tty
   env | grep -i hdr > /dev/tty
   echo '---------' > /dev/tty

   # extract everything before dash
   RECV_SPAN=${HDR_B3%%-*}

   if [ "${RECV_SPAN}" != "${TRACE_ID}" ]; then
      echo "Wrong header for b3 test" > /dev/tty
      STATUS="401 Test failed"
   fi

   echo -e "HTTP/1.1 ${STATUS}\r\nConnection: close\r\n\r\n"

   date
   sleep 1
   echo "B3 Header was: ${HDR_B3}"
   date
}

run_test() {
    curl --fail -v -H "${HEADER}" ${ENDPOINT_URL}/bar || failHttpd "Unable to download page with proxy enabled"
}

check_results() {
   echo "Checking that exactly two spans were created (client one and server one)"
   count '{' 2 # total two spans = one incoming and one outgoing

   echo "Checking that trace_id was properly parsed"
   check 'trace_id' ${TRACE_ID}

   # there should be at least one span type client
   grep "span kind     : Client" ${OUTPUT_SPANS} || fail "Span kind client not found"
   # there should be at least one span created (server one) with set parent_span_id as what we've sent
   grep "parent_span_id: ${TRACE_SPAN}" ${OUTPUT_SPANS} || fail "Inbound propagation failed - not found parent span_id"
   # now exclude it and find remaning one
   PROXY_SPAN_ID=`grep -v "parent_span_id: ${TRACE_SPAN}" ${OUTPUT_SPANS} | grep "parent_span_id:" | cut -d ':' -f 2 || fail "Inbound propagation failed - not found parent span_id"`
   # remaining parent_span_id should be span_id of the server one
   grep "span_id       :${PROXY_SPAN_ID}" ${OUTPUT_SPANS} || fail "Outbound propagation failed - it has not set proper parent span"
}

run $@
