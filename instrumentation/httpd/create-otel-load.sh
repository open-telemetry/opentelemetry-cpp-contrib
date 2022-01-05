#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

POSSIBLE_BUILD_OUTPUT=()
POSSIBLE_BUILD_OUTPUT+=("${SCRIPT_DIR}/bazel-out/k8-opt/bin/otel.so") # when build was done with Bazel
POSSIBLE_BUILD_OUTPUT+=("${SCRIPT_DIR}/build/otel_httpd_module.so") # when build was done with CMake

for LOCATE_OUTPUT in "${POSSIBLE_BUILD_OUTPUT[@]}"; do
   if [ -f ${LOCATE_OUTPUT} ]; then
      FOUND=${LOCATE_OUTPUT}
      echo Found file ${FOUND}
      break
   fi
done

if [ -z ${FOUND+x} ]; then
   echo "Binary module not found!"
   echo "Please run make build-cmake or make build-bazel first"
   exit 1
fi

# create configuration file for httpd (Apache)
cat << EOF > opentelemetry.load
# C++ Standard library
LoadFile /usr/lib/x86_64-linux-gnu/libstdc++.so.6

LoadModule otel_module $FOUND
EOF
