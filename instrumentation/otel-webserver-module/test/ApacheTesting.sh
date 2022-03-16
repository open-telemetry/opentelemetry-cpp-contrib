#!/bin/bash

# Bash script for testing Apache Server

# Extract the agent
tar -xf ../build/opentelemetry-webserver-sdk-x64-linux.tgz -C /opt

cd /opt/opentelemetry-webserver-sdk

echo "Installing webserver module"
./install.sh

# Create a appdynamics_agent.conf file
echo "Copying  agent config to appdynamics_agent file"
echo '
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_common.so
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_resources.so
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_trace.so
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_exporter_ostream_span.so
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_exporter_otprotocol.so

#Load the ApacheModule SDK
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_webserver_sdk.so
#Load the Apache Module. In this example for Apache 2.2
LoadModule otel_apache_module /opt/opentelemetry-webserver-sdk/WebServerModule/Apache/libmod_apache_otel22.so
ApacheModuleEnabled ON

#ApacheModule Otel Exporter details
ApacheModuleOtelSpanExporter otlp
ApacheModuleOtelExporterEndpoint docker.for.mac.localhost:4317

# SSL Certificates
#ApacheModuleOtelSslEnabled ON
#ApacheModuleOtelSslCertificatePath

ApacheModuleOtelSpanProcessor Batch
ApacheModuleOtelSampler AlwaysOn
ApacheModuleOtelMaxQueueSize 2048
ApacheModuleOtelScheduledDelayMillis 3000
ApacheModuleOtelExportTimeoutMillis 50000
ApacheModuleOtelMaxExportBatchSize 512

ApacheModuleServiceName DemoService
ApacheModuleServiceNamespace DemoServiceNamespace
ApacheModuleServiceInstanceId DemoInstanceId

ApacheModuleResolveBackends ON
ApacheModuleTraceAsError ON
#ApacheModuleWebserverContext DemoService DemoServiceNamespace DemoInstanceId

ApacheModuleSegmentType first
ApacheModuleSegmentParameter 2
' > /opt/opentelemetry_module.conf

#Include appdynamics_agent.conf file in httpd.conf
cp -f /opt/opentelemetry_module.conf /etc/httpd/conf.d/opentelemetry_module.conf

echo "Starting Apache Server"
apachectl restart # re-start the server

# run load
curl -m 5 http://localhost:80

echo "Stopping Apache Server"
apachectl stop # stop the server
