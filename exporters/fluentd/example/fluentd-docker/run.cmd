@echo off
pushd %~dp0
echo Building docker image in %CD%...
docker build --rm -t opentelemetry-fluentd ./
echo Starting OpenTelemetry-fluentd example...
docker run -p 24222:24222 -it -v %CD%\log:/fluentd/log opentelemetry-fluentd
popd
