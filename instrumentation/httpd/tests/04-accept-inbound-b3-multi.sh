#!/bin/bash

TEST_NAME="Check that incoming span is accepted with b3 multiline header (same trace_id and parent_span_id are set)"

. tools.sh

setup_test () {

cat << EOF > ${HTTPD_CONFIG}
OpenTelemetryExporter   file
OpenTelemetryPath ${OUTPUT_SPANS}
OpenTelemetryPropagators b3-multiheader
OpenTelemetryIgnoreInbound Off
EOF

}

TRACE_ID="00abba0000abba0000abba0000abba00"
TRACE_PARENT="aabbaabbaabbaabb"

HEADER_1="X-B3-TraceId: ${TRACE_ID}"
HEADER_2="X-B3-SpanId: ${TRACE_PARENT}"

run_test() {
    ${CURL_CMD} -H "${HEADER_1}" -H "${HEADER_2}" ${ENDPOINT_URL} || fail "Unable to download main page"
}

check_results() {
   echo Checking that exactly one span was created
   count '{' 1 # total one span

   echo "Checking that trace_id and span_id was properly parsed"
   check 'trace_id' ${TRACE_ID}
   check 'parent_span_id' ${TRACE_PARENT}
}

run $@
