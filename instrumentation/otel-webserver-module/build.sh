#!/bin/bash


git clone https://github.com/cisco-open/otel-webserver-module.git
cp -r /dependencies /otel-webserver-module/ 

cp -r /build-dependencies /otel-webserver-module/

# change gradle script to include build-dependencies folder 

# Building the otel-webserver-module

cd otel-webserver-module 
./gradlew assembleApacheModule -PbuildType=debug

#Changing the httpd.conf and Adding Opentelemetry.conf
cd build 
tar -xf opentelemetry-webserver-sdk-x64-linux.tgz
mv opentelemetry-webserver-sdk /opt/ 
cd ../
cp opentelemetry_module.conf /etc/httpd/conf.d/

# Installing

cd /opt/opentelemetry-webserver-sdk
./install.sh 

# Runing Apache

httpd
curl localhost:80



