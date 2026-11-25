#!/bin/bash
fileName=$1

sed -i "s/\(-L\/otel-webserver-module\/build\/linux-[a-z0-9_]*\/opentelemetry-webserver-sdk\/sdk_lib\/lib\)\ -lopentelemetry_webserver_sdk\ -ldl\ -lpthread\ -lcrypt\ -lpcre\ -lz\ \\\/\1\ -lopentelemetry_webserver_sdk\ -ldl\ -lrt\ -lpthread\ -lcrypt\ -lpcre\ -lz\ \\\/g" $fileName
sed -i "s/\(-L\/otel-webserver-module\/build\/linux-[a-z0-9_]*\/opentelemetry-webserver-sdk\/sdk_lib\/lib\)\ \\\/\1\ -lopentelemetry_webserver_sdk\ \\\/g" $fileName
