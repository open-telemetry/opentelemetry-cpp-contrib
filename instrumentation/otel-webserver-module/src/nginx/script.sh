#!/bin/bash
fileName=$1

sed -i "s/-L\/otel-webserver-module\/build\/linux-x64\/opentelemetry-webserver-sdk\/sdk_lib\/lib\ -lopentelemetry_webserver_sdk\ -ldl\ -lpthread\ -lcrypt\ -lpcre\ -lz\ \\\/-L\/otel-webserver-module\/build\/linux-x64\/opentelemetry-webserver-sdk\/sdk_lib\/lib\ -lopentelemetry_webserver_sdk\ -ldl\ -lrt\ -lpthread\ -lcrypt\ -lpcre\ -lz\ \\\/g" $fileName
sed -i "s/-L\/otel-webserver-module\/build\/linux-x64\/opentelemetry-webserver-sdk\/sdk_lib\/lib\ \\\/-L\/otel-webserver-module\/build\/linux-x64\/opentelemetry-webserver-sdk\/sdk_lib\/lib\ -lopentelemetry_webserver_sdk\ \\\/g" $fileName
