
SCRIPT_NAME=$(basename "$0")

HTTPD_CONFIG=${TEST_DIR_TMP}/extra-test-configuration.conf
HTTPD_ERRLOG=/var/log/apache2/error.log

ENDPOINT_ADDR=127.0.0.1
ENDPOINT_PORT=80
ENDPOINT_URL=http://${ENDPOINT_ADDR}
PROXY_PORT=1500
ENDPOINT_PROXY=http://127.0.0.1:${PROXY_PORT}

CURL_TIMEOUT=2 # two seconds
CURL_CMD="curl --silent --show-error --fail -m ${CURL_TIMEOUT} -o /dev/null -v "

OUTPUT_SPANS=/tmp/text-${SCRIPT_NAME}.spans

EXTRA_HTTPD_MODS=""

fail () {
    printf 'FAIL: %s\n' "$1" >&2  ## Send message to stderr
    echo "ERROR TEST FAILED" >&2
    exit "${2-1}"  ## Return a code specified by $2 or 1 by default.
}

failHttpd () {
    printf 'FAIL: %s\n' "$1" >&2  ## Send message to stderr. Exclude >&2 if you don't want it that way.
    echo "--- Below is httpd extra configuration used for this test with line numbers"
    nl -b a ${HTTPD_CONFIG}
    echo "--- Below is httpd last 10 entires from error log"
    tail -n 10 ${HTTPD_ERRLOG}
    echo "ERROR TEST FAILED" >&2
    exit "${2-1}"
}

# count that given string occurs exactly n times
count() {
    TOTAL=`grep -c "$1" ${OUTPUT_SPANS}`
    if [ "$TOTAL" -ne $2 ]; then
       echo "---"
       cat ${OUTPUT_SPANS}
       echo "---"
       fail "Total number of $1 is not $2 but $TOTAL"
    fi
    echo OK Found $TOTAL occurence\(s\) of "$1"
}

# returns one span attribute
getSpanAttr() {
  LINE=`grep "attributes    :" ${OUTPUT_SPANS} -A 20 | grep "	$1" `
  VALUE=`echo $LINE | cut -d ':' -f 2-`
  VALUE="${VALUE## }"
  echo $VALUE
}

# returns one span field
getSpanField() {
  WHICHONE=${2-1}
  LINE=`grep -m ${WHICHONE} "$1" ${OUTPUT_SPANS} | tail -n 1`
  VALUE=`echo $LINE | cut -d ':' -f 2-`
  VALUE="${VALUE## }"
  echo $VALUE
}

# check that in span we have $1: $2
check() {
  WHICHONE=${3-1}
  VALUE=`getSpanField "$1" ${WHICHONE}`
  if [ "$VALUE" != "$2" ]; then
        echo "---"
        cat ${OUTPUT_SPANS}
        echo "---"
        echo $LINE
        fail "Value for \"$1\" is not \"$2\" but \"$VALUE\""
  fi
  echo OK - \"$1\" found with \"$2\"
}

# this one should be redefined in each test
run_test() {
  :
}

# this one should be redefined in each test
check_results() {
  :
}

PROXY_PID_FILE=/tmp/proxy-listen-${PROXY_PORT}
NETCAT=/bin/nc.traditional

start_proxy () {
  echo $$ > ${PROXY_PID_FILE}
  echo "Starting proxy on port ${PROXY_PORT} PID:$$ PID-File: ${PROXY_PID_FILE}"
  # as long as pid file exists and contains our PID number handle incoming requests with netcat
  while [ "`cat ${PROXY_PID_FILE} 2> /dev/null`" == "$$" ]; do
    ${NETCAT} -w 1 -l -p ${PROXY_PORT} -c "$0 handle_proxy" 2>/dev/null
  done
  echo "Stopping proxy on port ${PROXY_PORT} PID:$$ PID-File: ${PROXY_PID_FILE}"
}

stop_proxy () {
    rm -rf ${PROXY_PID_FILE}
}

# wrapper for netcat
handle_proxy_request () {
  # read first HTTP line
  read -t 0.1 REQUEST_LINE
  REQ_URL=${REQUEST_LINE% *}
  REQ_METHOD=${REQ_URL% *}
  REQ_URL=${REQ_URL#* }

  # put all headers into env vars which start from HDR_ for simplicity
  while read -t 0.1 HEADER
  do
    HEADER=${HEADER:0:-1}
    if [ "$HEADER" = "" ]; then
        break;
    fi
    HEADER_NAME=${HEADER%%:*}
    HEADER_NAME=${HEADER_NAME//-} # User-Agent => UserAgent
    HEADER_NAME=${HEADER_NAME^^}  # UserAgent => USERAGENT

    HEADER_VALUE=${HEADER#*: }
    export HDR_${HEADER_NAME}="${HEADER_VALUE}"
  done

  # now call handler function
  proxy
}

# execute entire test
run() {
   if [ "$1" == "start_proxy" ]; then
     start_proxy
     exit 0
   fi
   if [ "$1" == "handle_proxy" ]; then
     handle_proxy_request
     exit 0
   fi
   echo "------------------------------ Starting test $SCRIPT_NAME"
   echo "----- Info: ${TEST_NAME}"

   apache2ctl stop

   # remove old spans (if any)
   rm -rf ${OUTPUT_SPANS}

   setup_test

   # start proxy (netcat based) only when handler function was defined inside test script
   if type proxy &>/dev/null ; then
      rm -rf ${PROXY_PID_FILE}
      $0 start_proxy &
      trap stop_proxy EXIT
      # wait for proxy to be started
      echo "Waiting for $! to be ready"
      while [ "`cat ${PROXY_PID_FILE} 2> /dev/null`" != "$!" ]; do sleep 0.1; done
      echo "Proxy process already started"
   fi

   # enable extra modules (if needed for this test)
   if [ "${EXTRA_HTTPD_MODS}" != "" ]; then
      a2enmod ${EXTRA_HTTPD_MODS} || failHttpd "Failed enable httpd mods ${EXTRA_HTTPD_MODS}"
   fi

   # now check configuration
   apache2ctl -t -c "Include ${HTTPD_CONFIG}" || failHttpd "Apache configtest failed"

   # run apache
   apache2ctl -k start -c "Include ${HTTPD_CONFIG}" || failHttpd "Apache start failed"

   # now run test
   run_test

   # stop apache - this is important as this flushes span file
   apache2ctl -k stop -c "Include ${HTTPD_CONFIG}" || failHttpd "Apache stop failed"

   # stop proxy if it was used by this test
   if type proxy &>/dev/null ; then
      echo "Waiting for proxy to stop (PID $!)"
      rm -rf ${PROXY_PID_FILE}
      wait $!
   fi

   # disable extra modules
   if [ "${EXTRA_HTTPD_MODS}" != "" ]; then
      # reverse list of modules so proxy proxy_http becomes proxy_http proxy
      DISABLE_MODS=`echo ${EXTRA_HTTPD_MODS} | tac -s' '`
      echo "Disabling mods ${DISABLE_MODS}"
      a2dismod ${DISABLE_MODS} || failHttpd "Failed disable httpd mods ${EXTRA_HTTPD_MODS}"
   fi

   # and check results
   check_results && echo "Test ${SCRIPT_NAME} PASSED OK"
}
