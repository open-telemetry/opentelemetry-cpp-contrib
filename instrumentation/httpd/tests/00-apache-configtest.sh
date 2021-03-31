#!/bin/bash

TEST_NAME="Check that OpenTelemetry module can be loaded by httpd (Apache)"

. tools.sh

setup_test () {

cat << EOF > ${HTTPD_CONFIG}
<IfModule !mod_otel.cpp>
Error "OpenTelemetry module is required to run all tests. Run a2enmod otel"
</IfModule>
EOF

}

run $@
