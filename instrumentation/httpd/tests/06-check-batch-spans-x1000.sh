#!/bin/bash

TEST_NAME="Check that 1000 requests creates 1000 spans with batch processing"

. tools.sh

TOTAL_SPANS=1000

setup_test () {

cat << EOF > ${HTTPD_CONFIG}
OpenTelemetryExporter   file
OpenTelemetryPath ${OUTPUT_SPANS}
OpenTelemetryBatch 10 5000 5
EOF

}

run_test() {
   httperf --timeout=5 --client=0/1 --server=${ENDPOINT_ADDR} --port=${ENDPOINT_PORT} --uri=/ --rate=150 --send-buffer=4096 --recv-buffer=16384 --num-conns=${TOTAL_SPANS} --num-calls=1
}

check_results() {
   echo Checking that all ${TOTAL_SPANS} were created
   count '{' ${TOTAL_SPANS} # span count is good
}

run $@
