#!/bin/bash

TEST_NAME="Check that incoming span is accepted with b3 format (same trace_id and parent_span_id are set)"

. tools.sh

setup_test () {

cat << EOF > ${HTTPD_CONFIG}
OpenTelemetryExporter   file
OpenTelemetryPath ${OUTPUT_SPANS}
OpenTelemetryPropagators b3
OpenTelemetryIgnoreInbound Off
EOF

}

TRACE_ID="0abcdefabcdefabcdefabcdefabcdef0"
TRACE_PARENT="0123456789abcdef"
TRACE_FLAGS="1"

HEADER="b3: ${TRACE_ID}-${TRACE_PARENT}-${TRACE_FLAGS}"

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
