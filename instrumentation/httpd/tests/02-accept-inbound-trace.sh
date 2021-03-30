#!/bin/bash

TEST_NAME="Check that incoming span is accepted (same trace_id and parent_span_id are set)"

. tools.sh

setup_test () {

cat << EOF > ${HTTPD_CONFIG}
OpenTelemetryExporter   file
OpenTelemetryPath ${OUTPUT_SPANS}
OpenTelemetryPropagators trace-context
OpenTelemetryIgnoreInbound Off
EOF

}

TRACE_VER="00"
TRACE_ID="00112233445566778899aabbccddeeff"
TRACE_PARENT="0101010101010101"
TRACE_FLAGS="01"

HEADER="traceparent: ${TRACE_VER}-${TRACE_ID}-${TRACE_PARENT}-${TRACE_FLAGS}"

run_test() {
    ${CURL_CMD} -H "${HEADER}" ${ENDPOINT_URL} || fail "Unable to download main page"
}

check_results() {
   echo Checking that exactly one span was created
   count '{' 1 # total one span

   echo "Checking that trace_id and span_id was properly parsed"
   check 'trace_id' ${TRACE_ID}
   check 'parent_span_id' ${TRACE_PARENT}
}

run $@
