#!/bin/bash

# Bash script for testing Apache Server

# Extract the agent
tar -xf ../build/opentelemetry-webserver-sdk-x64-linux.tgz -C /opt

cd /opt/opentelemetry-sdk-native

echo "Installing Apache Agent"
./install.sh

# Create a opentelemetry_agent.conf file
echo "Copying  agent config to opentelemetry_agent file"
echo '
NginxModuleEnabled ON;

# Otel Exporter details
NginxModuleOtelSpanExporter OTLP;
NginxModuleOtelExporterEndpoint example.com:14250;

NginxModuleOtelSpanProcessor Batch;
NginxModuleOtelSampler AlwaysOn;

NginxModuleServiceName cart;
NginxModuleServiceNamespace e-commerce;
NginxModuleServiceInstanceId 71410b7dec09;

NginxModuleOtelMaxQueueSize 1024;
NginxModuleOtelScheduledDelayMillis 3000;
NginxModuleOtelExportTimeoutMillis 30000;
NginxModuleOtelMaxExportBatchSize 1024;

NginxModuleResolveBackends ON;

NginxModuleTraceAsError ON;

NginxModuleReportAllInstrumentedModules OFF;

#NginxModuleWebserverContext electronics e-commerce 71410b7jan13;

NginxModuleMaskCookie ON;
NginxModuleCookieMatchPattern PHPSESSID;
NginxModuleMaskSmUser ON;

NginxModuleDelimiter /;
NginxModuleSegment 2,3;
NginxModuleMatchfilter CONTAINS;
NginxModuleMatchpattern myapp;
' > /opt/opentelemetry_agent.conf

# Overwrite nginx.conf file and include opentelemetry_agent.conf and ngx_http_opentelemetry_module.so in it
echo '
user  nobody;
worker_processes  1;

error_log  /var/log/nginx/error.log notice;
pid        /var/run/nginx.pid;

load_module /opt/opentelemetry-sdk-native/WebServerAgent/Nginx/ngx_http_opentelemetry_module.so;

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

    include /opt/opentelemetry_agent.conf;

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
