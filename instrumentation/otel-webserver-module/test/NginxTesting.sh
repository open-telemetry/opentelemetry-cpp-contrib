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
AppDynamicsEnabled ON;

#AppDynamics Otel Exporter details
AppDynamicsOtelSpanExporter OTLP;
AppDynamicsOtelExporterEndpoint example.com:14250;

AppDynamicsOtelSpanProcessor Batch;
AppDynamicsOtelSampler AlwaysOn;

AppDynamicsServiceName cart;
AppDynamicsServiceNamespace e-commerce;
AppDynamicsServiceInstanceId 71410b7dec09;

AppDynamicsOtelMaxQueueSize 1024;
AppDynamicsOtelScheduledDelayMillis 3000;
AppDynamicsOtelExportTimeoutMillis 30000;
AppDynamicsOtelMaxExportBatchSize 1024;

AppDynamicsResolveBackends ON;

AppDynamicsTraceAsError ON;

AppDynamicsReportAllInstrumentedModules OFF;

#AppDynamicsWebserverContext electronics e-commerce 71410b7jan13;

AppDynamicsMaskCookie ON;
AppDynamicsCookieMatchPattern PHPSESSID;
AppDynamicsMaskSmUser ON;

AppDynamicsDelimiter /;
AppDynamicsSegment 2,3;
AppDynamicsMatchfilter CONTAINS;
AppDynamicsMatchpattern myapp;
' > /opt/appdynamics_agent.conf

# Overwrite nginx.conf file and include appdynamics_agent.conf and ngx_http_appdynamics_module.so in it
echo '
user  nobody;
worker_processes  1;

error_log  /var/log/nginx/error.log notice;
pid        /var/run/nginx.pid;

load_module /opt/appdynamics-sdk-native/WebServerAgent/Nginx/ngx_http_appdynamics_module.so;

events {
    worker_connections  1024;
}

http {
    include       /etc/nginx/mime.types;
    default_type  application/octet-stream;

    log_format  main " $remote_addr - $remote_user [$time_local] $status " ;

    access_log  /var/log/nginx/access.log  main;

    sendfile        on;
    keepalive_timeout  65;

    include /opt/appdynamics_agent.conf;

    server {
        listen 8080;
    }
}
' > /etc/nginx/nginx.conf 

echo "Starting Nginx Server"
pkill nginx
nginx # re-start the server

# run load
curl -m 5 http://localhost:80

echo "Stopping Nginx Server"
pkill nginx # stop the server
