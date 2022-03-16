#!/bin/bash

# Bash script for testing Apache Server

# Extract the agent
tar -xf ../build/appdynamics-webserver-sdk-x64-linux.tgz -C /opt

cd /opt/appdynamics-sdk-native

echo "Installing Apache Agent"
./install.sh

# Create a appdynamics_agent.conf file
echo "Copying  agent config to appdynamics_agent file"
echo '
LoadFile /opt/appdynamics-sdk-native/sdk_lib/lib/libopentelemetry_common.so
LoadFile /opt/appdynamics-sdk-native/sdk_lib/lib/libopentelemetry_resources.so
LoadFile /opt/appdynamics-sdk-native/sdk_lib/lib/libopentelemetry_trace.so
LoadFile /opt/appdynamics-sdk-native/sdk_lib/lib/libopentelemetry_otlp_recordable.so
LoadFile /opt/appdynamics-sdk-native/sdk_lib/lib/libopentelemetry_exporter_ostream_span.so
LoadFile /opt/appdynamics-sdk-native/sdk_lib/lib/libopentelemetry_exporter_otlp_grpc.so

#Load the AppDynamics SDK
LoadFile /opt/appdynamics-sdk-native/sdk_lib/lib/libappdynamics_native_sdk.so

#Load the Apache Agent. In this example for Apache 2.2
LoadModule appdynamics_module /opt/appdynamics-sdk-native/WebServerAgent/Apache/libmod_appdynamics22.so

AppDynamicsEnabled ON

#AppDynamics Otel Exporter details
AppDynamicsOtelSpanExporter OTLP
AppDynamicsOtelExporterEndpoint example.com:14250

AppDynamicsOtelSpanProcessor Batch
AppDynamicsOtelSampler AlwaysOn

AppDynamicsServiceName cart
AppDynamicsServiceNamespace e-commerce
AppDynamicsServiceInstanceId 71410b7dec09

AppDynamicsOtelMaxQueueSize 1024
AppDynamicsOtelScheduledDelayMillis 3000
AppDynamicsOtelExportTimeoutMillis 30000
AppDynamicsOtelMaxExportBatchSize 1024

AppDynamicsResolveBackends ON

AppDynamicsTraceAsError ON

AppDynamicsReportAllInstrumentedModules OFF

AppDynamicsWebserverContext electronics e-commerce 71410b7jan13

AppDynamicsMaskCookie ON
AppDynamicsCookieMatchPattern PHPSESSID
AppDynamicsMaskSmUser ON

AppDynamicsDelimiter /
AppDynamicsSegment 2,3
AppDynamicsMatchfilter CONTAINS
AppDynamicsMatchpattern myapp
' >> /opt/appdynamics_agent.conf

#Include appdynamics_agent.conf file in httpd.conf
echo 'Include /opt/appdynamics_agent.conf' >> /etc/httpd/conf/httpd.conf

echo "Starting Apache Server"
apachectl restart # re-start the server

# run load
curl -m 5 http://localhost:80

echo "Stopping Apache Server"
apachectl stop # stop the server
