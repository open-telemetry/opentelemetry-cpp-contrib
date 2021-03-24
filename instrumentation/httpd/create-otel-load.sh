#!/bin/sh

# create configuration file for httpd (Apache)
cat << EOF > opentelemetry.load
# C++ Standard library
LoadFile /usr/lib/x86_64-linux-gnu/libstdc++.so.6

LoadModule otel_module $PWD/bazel-out/k8-opt/bin/otel.so
EOF
