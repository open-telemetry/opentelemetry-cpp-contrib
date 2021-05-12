#!/bin/bash

TEST_NAME="Check that span is created"

. tools.sh

setup_test () {

cat << EOF > ${HTTPD_CONFIG}
OpenTelemetryExporter   file
OpenTelemetryPath ${OUTPUT_SPANS}
EOF

}

run_test() {
    ${CURL_CMD} ${ENDPOINT_URL} || fail "Unable to download main page"
}


check_results() {
   echo Checking that exactly one span was created
   count '{' 1 # total one span

   echo Checking span fields
   check name 'HTTP GET'
   check 'span kind' Server
   [ "`getSpanField span_id`" != "`getSpanField parent_span_id`" ] || fail "Bad span: span.id same as parent span.id"

   echo Checking span attributes

   [[ "`getSpanAttr 'http.method'`" == "GET" ]] || fail "Bad span attribiute http.method ${SPAN_ATTRS[http.method]}"
   [[ "`getSpanAttr 'http.target'`" == "/" ]] || fail "Bad span attribiute http.target ${SPAN_ATTRS[http.target]}"
   [[ "`getSpanAttr 'http.status_code'`" == "200" ]] || fail "Bad span attribiute http.status_code ${SPAN_ATTRS[http.status_code]}"
   [[ "`getSpanAttr 'http.flavor'`" == "1.1" ]] || fail "Bad span attribiute http.flavor ${SPAN_ATTRS[http.flavor]}"
   [[ "`getSpanAttr 'http.scheme'`" == "http" ]] || fail "Bad span attribiute http.scheme ${SPAN_ATTRS[http.scheme]}"
}

run $@
