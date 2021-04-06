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
   declare -A SPAN_ATTRS
   # transforms "http.method: GET, http.flavor: http, ..." into SPAN_ATTRS[http.method] = GET
   IFS=',' read -ra my_array <<< "`getSpanField attributes`"
   for i in "${my_array[@]}"; do
     TMP_KEY=${i%%:*}
     TMP_KEY=${TMP_KEY# }
     TMP_VAL=${i##*:}
     TMP_VAL=${TMP_VAL:1}
     SPAN_ATTRS[${TMP_KEY}]="${TMP_VAL}"
   done

   [[ "${SPAN_ATTRS[http.method]}" == "GET" ]] || fail "Bad span attribiute http.method ${SPAN_ATTRS[http.method]}"
   [[ "${SPAN_ATTRS[http.target]}" == "/" ]] || fail "Bad span attribiute http.target ${SPAN_ATTRS[http.target]}"
   [[ "${SPAN_ATTRS[http.status_code]}" == "200" ]] || fail "Bad span attribiute http.status_code ${SPAN_ATTRS[http.status_code]}"
   [[ "${SPAN_ATTRS[http.flavor]}" == "1.1" ]] || fail "Bad span attribiute http.flavor ${SPAN_ATTRS[http.flavor]}"
   [[ "${SPAN_ATTRS[http.scheme]}" == "http" ]] || fail "Bad span attribiute http.scheme ${SPAN_ATTRS[http.scheme]}"
}

run $@
