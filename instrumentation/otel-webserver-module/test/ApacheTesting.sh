#!/bin/bash

# Bash script for running Apache Server

# Extract the agent

targetSystem=$1

OTelApacheModule="/opt/opentelemetry-webserver-sdk/WebServerModule/Apache/libmod_apache_otel22.so"
if [ $targetSystem = "ubuntu" ] || [ $targetSystem = "centos7" ]; then
	OTelApacheModule="/opt/opentelemetry-webserver-sdk/WebServerModule/Apache/libmod_apache_otel.so"
fi


tar -xf ../build/opentelemetry-webserver-sdk-x64-linux.tgz -C /opt

cd /opt/opentelemetry-webserver-sdk

echo "Installing webserver module"
./install.sh

# Create a opentelemetry_module.conf file
echo "Copying  agent config to opentelemetry_module file"
echo '
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_common.so
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_resources.so
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_trace.so
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_otlp_recordable.so
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_exporter_ostream_span.so
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_exporter_otlp_grpc.so

#Load the ApacheModule SDK
LoadFile /opt/opentelemetry-webserver-sdk/sdk_lib/lib/libopentelemetry_webserver_sdk.so
#Load the Apache Module. In this example for Apache 2.2
LoadModule otel_apache_module '$OTelApacheModule'
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

targetConfFile='/etc/httpd/conf.d/opentelemetry_module.conf'
if [ $targetSystem = "ubuntu" ]; then
	targetConfFile='/etc/apache2/opentelemetry_module.conf'
elif [ $targetSystem = "centos7" ]; then
	targetConfFile='/etc/httpd/conf/opentelemetry_module.conf'
fi

rm -f $targetConfFile
cp -f /opt/opentelemetry_module.conf $targetConfFile

if [ $targetSystem = "ubuntu" ]; then
	if ! grep -Fxq "Include opentelemetry_module.conf" /etc/apache2/apache2.conf; then
		echo 'Include opentelemetry_module.conf' >> /etc/apache2/apache2.conf
	fi

elif [ $targetSystem = "centos7" ]; then
	if ! grep -Fxq "Include opentelemetry_module.conf" /etc/httpd/conf/httpd.conf; then
		echo 'Include opentelemetry_module.conf' >> /etc/httpd/conf/httpd.conf
	fi
fi

echo "Starting Apache Server"
apachectl restart # re-start the server

# run load
curl -m 5 http://localhost:80

echo "Stopping Apache Server"
apachectl stop # stop the server
