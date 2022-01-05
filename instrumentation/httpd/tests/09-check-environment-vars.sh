#!/bin/bash

TEST_NAME="Check that tracestate and traceparent are available as variables"

. tools.sh

EXTRA_LOG_FILE=/tmp/apache-test-$$.log

setup_test () {

EXTRA_HTTPD_MODS="headers"

# here we are setting apache log to record environment variables created by mod_otel
# header set is just an extra debug information so it is also
# visible on standard output when running test during CI/CD cycle
cat << EOF > ${HTTPD_CONFIG}
OpenTelemetryExporter   file
OpenTelemetryPath ${OUTPUT_SPANS}
Header set x-env-spanid %{OTEL_SPANID}e
Header set x-env-traceid %{OTEL_TRACEID}e
Header set x-env-traceflags %{OTEL_TRACEFLAGS}e
Header set x-env-tracestate %{OTEL_TRACESTATE}e
LogFormat "span=%{OTEL_SPANID}e trace=%{OTEL_TRACEID}e flags=%{OTEL_TRACEFLAGS}e state=%{OTEL_TRACESTATE}e" otel_format
GlobalLog ${EXTRA_LOG_FILE} otel_format
EOF

}

run_test() {
  ${CURL_CMD} ${ENDPOINT_URL} || fail "Unable to download main page"
}

check_results() {
   echo Checking that log file contains information
   grep --color -E "span=[0-9a-f]{16}" ${EXTRA_LOG_FILE} || fail "SpanID not found in log file"
   grep --color -E "trace=[0-9a-f]{32}" ${EXTRA_LOG_FILE} || fail "TraceID not found in log file"
   grep --color -E "flags=1" ${EXTRA_LOG_FILE} || fail "TraceFlags not found in log file"
}

teardown_test() {
   rm -rf ${OUTPUT_SPANS} ${EXTRA_LOG_FILE}
}

run $@
