#!/usr/bin/bash
pushd `pwd`
echo Building docker image in current directory
docker build --rm -t opentelemetry-fluentd ./
echo Starting OpenTelemetry-fluentd example...
docker run -p 24222:24222 -it -v /tmp/log:/tmp/fluentd/log opentelemetry-fluentd
popd
