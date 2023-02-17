# otel-webserver-module

The OTEL webserver module comprises of both Apache and Nginx instrumentation.

## Apache Webserver Module

The Apache Module enables tracing of incoming requests to the server by injecting instrumentation into the Apache server at runtime. Along with the above mentioned capability, the Apache web server module has capabilities to capture the response time of many modules (including mod_proxy) involved in an incoming request, thereby including the hierarchical time consumption by each module.

### Module wise Monitoring

Monitoring individual modules is crucial to the instrumentation of Apache web server. As the HTTP request flows through individual modules, delay in execution or errors might occur at any of the modules involved in the request. To identify the root cause of any delay or errors in request processing, module wise information (such as response time of individual modules) would enhance the debuggability of the Apache web server.

#### Some of the Modules monitored by Apache Module
| Some of the use cases handled by Apache Module | Module used |
| ---------------------------------------------- | ----------- |
| Monitor each module in the login phase         | mod_sso     |
| Monitor php application running on the same machine | mod_php|
| Monitor requests to remote server              | mod_dav     |
| Monitor reverse proxy requests                 | mod_proxy   |
| Monitor the reverse proxy load balancer        | mod_proxy_balancer |

### Third Party Dependencies

| Library                                        | Present Version |
| ---------------------------------------------- | -----------     |
| Apache-log4cxx                                 | 0.11.0          |
| Apr                                            | 1.7.0           |
| Apr-util                                       | 1.6.1           |
| Expat                                          | 2.3.0           |
| Boost                                          | 1.75.0          |
| Opentelemetry - C++ SDK                        | 1.2.0      |
| Googletest                                     | 1.10.0          |

*There are some libraries which are just used to generate Apache Header files

| Library                                        | Present Version |
| ---------------------------------------------- | -----------     |
| Httpd                                          | 2.4.23, 2.2.31          |
| Apr                                            | 1.7.0           |
| Apr-util                                       | 1.6.1           |

### Configuration
| Configuration Directives                       |  Default Values |  Remarks                                   |
| ---------------------------------------------- | --------------- | ------------------------------------------ |
|*ApacheModuleEnabled*                           | ON              | OPTIONAL: Needed for instrumenting Apache Webserver |
|*ApacheModuleOtelSpanExporter*                 | otlp             | OPTIONAL: Specify the span exporter to be used. Supported values are "otlp" and "ostream". All other supported values would be added in future. |
|*ApacheModuleOtelExporterEndpoint:*             |                 | REQUIRED: The endpoint otel exporter exports to. Example "docker.for.mac.localhost:4317" |
|*ApacheModuleOtelSpanProcessor*                 | batch           | OPTIONAL: Specify the processor to select to. Supported values are "simple" and "batch".|
|*ApacheModuleOtelSampler*                       | AlwaysOn        | OPTIONAL: Supported values are "AlwaysOn" and "AlwaysOff" |
|*ApacheModuleOtelMaxQueueSize*                  | 2048            | OPTIONAL: The maximum queue size. After the size is reached spans are dropped|
|*ApacheModuleOtelScheduledDelayMillis*          | 5000            | OPTIONAL: The delay interval in milliseconds between two consecutive exports|
|*ApacheModuleOtelExportTimeoutMillis*           | 30000           | OPTIONAL: How long the export can run in milliseconds before it is cancelled|
|*ApacheModuleOtelMaxExportBatchSize*            | 512             | OPTIONAL: The maximum batch size of every export. It must be smaller or equal to maxQueueSize |
|*ApacheModuleServiceName*                       |                 | REQUIRED: A namespace for the ServiceName|
|*ApacheModuleServiceNamespace*                  |                 | REQUIRED: Logical name of the service |
|*ApacheModuleServiceInstanceId*                 |                 | REQUIRED: The string ID of the service instance |
|*ApacheModuleTraceAsError*                      |                 | OPTIONAL: Trace level for logging to Apache log|
|*ApacheModuleWebserverContext*                  |                 | OPTIONAL: Takes 3 values(space-seperated) ServiceName, ServiceNamespace and ServiceInstanceId|
|*ApacheModuleSegmentType*                       |                 | OPTIONAL: Specify the string (FIRST/LAST/CUSTOM) to be filtered for Span Name Creation|
|*ApacheModuleSegmentParameter*                  |                 | OPTIONAL: Specify the segment count or segment numbers that you want to display for Span Creation|
|*ApacheModuleOtelExporterHeaders*               |                 | OPTIONAL: OTEL Exporter header info or Metadata like API key for OTLP endpoint. a list of key value pairs, and these are expected to be represented in a format matching to the W3C Correlation-Context, except that additional semi-colon delimited metadata is not supported, i.e.: key1=value1,key2=value2. |

A sample configuration is mentioned in [opentelemetry_module.conf](https://github.com/open-telemetry/opentelemetry-cpp-contrib/blob/main/instrumentation/otel-webserver-module/opentelemetry_module.conf)

### Build and Installation
#### Prerequisites
- Docker Desktop should be installed on the system

#### Platform Supported
- The build is supported for **x86-64** platforms.
- OS support: **Centos6**, **Centos7, ubuntu20.04**.

#### Automatic build and Installation

We will use Docker to run the Module. First, it is to be made sure that the Docker is up and running. 
Then execute the following commands -:
```
docker-compose --profile default build
docker-compose --profile default up
```
Alternatively, replace the value of *profile* from **'default'** to **'centos7'** or **'ubuntu20.04'** to build in respective supported platforms.

This would start the container alongwith the the Opentelemetry Collector and Zipkin. You can check the traces on Zipkin dashboard by checking the port number of Zipkin using ```docker ps``` command. Multiple requests can be sent using the browser.

#### Manual build and Installation

The artifact can be either downloaded or built manually.

##### Generate the artifact manually

We will use Docker to run the Module. First, it is to be made sure that the Docker is up and running. 
Then execute the following commands -:
```
docker-compose --profile default build
docker-compose --profile default up
```
Next, login into the Docker container. 
After going inside the container run the following commands ```cd \otel-webserver-module```. After making code changes the build and installation can be done by running ```./install.sh```.

The build file can be copied at a suitable location in User's system by running the command ```docker cp <container_name>:/otel-webserver-module/build/opentelemetry-webserver-sdk-x64-linux.tgz  <desired_location>```.

##### Download the artifact

The artifact can also be downloaded from either of below links
- [Releases](https://github.com/open-telemetry/opentelemetry-cpp-contrib/releases) - Make sure to download from releases/tags having ```webserver/vXX.XX.XX``` e.g [webserver/v1.0.0](https://github.com/open-telemetry/opentelemetry-cpp-contrib/releases/tag/webserver%2Fv1.0.0)
- [GitHub Actions](https://github.com/open-telemetry/opentelemetry-cpp-contrib/actions/workflows/webserver.yml) - Click on any top successful workflow runs on main branch and the artifact would be available for download.

##### Installation Steps

The installation steps can be referred from [instrument-apache-httpd-server](https://opentelemetry.io/blog/2022/instrument-apache-httpd-server/#installing-opentelemetry-module-in-target-system).
Please ignore the initial steps which talks about generating the artifact locally.

## Nginx Webserver Module

Similar to Apache, Nginx Webserver Module also enables tracing of incoming requests to the server by injecting instrumentation into the Nginx server at runtime. It also captures the response time of the individual modules involved in the request processing.

### Module Wise Monitoring

Currently, Nginx Webserver module monitores some fixed set of modules, which gets involved in the request processing. Following are the set of modules monitored:
* ngx_http_realip_module
* ngx_http_rewrite_module
* ngx_http_limit_conn_module
* ngx_http_limit_req_module
* ngx_http_auth_request_module
* ngx_http_auth_basic_module
* ngx_http_access_module
* ngx_http_static_module
* ngx_http_gzip_static_module
* ngx_http_dav_module
* ngx_http_autoindex_module
* ngx_http_index_module
* ngx_http_random_index_module
* ngx_http_try_files_module
* ngx_http_mirror_module

### Third Party Dependencies

| Library                                        | Present Version |
| ---------------------------------------------- | -----------     |
| Apache-log4cxx                                 | 0.11.0          |
| Apr                                            | 1.7.0           |
| Apr-util                                       | 1.6.1           |
| Expat                                          | 2.3.0           |
| Boost                                          | 1.75.0          |
| Opentelemetry - C++ SDK                        | 1.2.0           |
| Googletest                                     | 1.10.0          |
| Pcre                                           | 8.44            |

*There are some libraries which are just used to generate Apache Header files

| Library                                        | Present Version |
| ---------------------------------------------- | -----------     |
| Nginx                                          | 1.22.0, 1.23.0,1.23.1          |
| Apr                                            | 1.7.0           |
| Apr-util                                       | 1.6.1           |

### Configuration
| Configuration Directives                       |  Default Values |  Remarks                                   |
| ---------------------------------------------- | --------------- | ------------------------------------------ |
|*NginxModuleEnabled*                           | ON              | OPTIONAL: Needed for instrumenting Nginx Webserver |
|*NginxModuleOtelSpanExporter*                 | otlp             | OPTIONAL: Specify the span exporter to be used. Supported values are "otlp" and "ostream". All other supported values would be added in future. |
|*NginxModuleOtelExporterEndpoint:*             |                 | REQUIRED: The endpoint otel exporter exports to. Example "docker.for.mac.localhost:4317" |
|*NginxModuleOtelSpanProcessor*                 | batch           | OPTIONAL: Specify the processor to select to. Supported values are "simple" and "batch".|
|*NginxModuleOtelSampler*                       | AlwaysOn        | OPTIONAL: Supported values are "AlwaysOn" and "AlwaysOff" |
|*NginxModuleOtelMaxQueueSize*                  | 2048            | OPTIONAL: The maximum queue size. After the size is reached spans are dropped|
|*NginxModuleOtelScheduledDelayMillis*          | 5000            | OPTIONAL: The delay interval in milliseconds between two consecutive exports|
|*NginxModuleOtelExportTimeoutMillis*           | 30000           | OPTIONAL: How long the export can run in milliseconds before it is cancelled|
|*NginxModuleOtelMaxExportBatchSize*            | 512             | OPTIONAL: The maximum batch size of every export. It must be smaller or equal to maxQueueSize |
|*NginxModuleServiceName*                       |                 | REQUIRED: A namespace for the ServiceName|
|*NginxModuleServiceNamespace*                  |                 | REQUIRED: Logical name of the service |
|*NginxModuleServiceInstanceId*                 |                 | REQUIRED: The string ID of the service instance |
|*NginxModuleTraceAsError*                      |                 | OPTIONAL: Trace level for logging to Apache log|
|*NginxModuleWebserverContext*                  |                 | OPTIONAL: Takes 3 values(space-seperated) ServiceName, ServiceNamespace and ServiceInstanceId|
|*NginxModuleSegmentType*                       |                 | OPTIONAL: Specify the string (FIRST/LAST/CUSTOM) to be filtered for Span Name Creation|
|*NginxModuleSegmentParameter*                  |                 | OPTIONAL: Specify the segment count or segment numbers that you want to display for Span Creation|
|*NginxModuleRequestHeaders*                    |                 | OPTIONAL: Specify the request headers to be captured in the span attributes. The headers are Case-Sensitive and should be comma-separated. e.g.```NginxModuleRequestHeaders               Accept-Charset,Accept-Encoding,User-Agent;```|
|*NginxModuleResponseHeaders*                   |                  | OPTIONAL: Specify the response headers to be captured in the span attributes. The headers are Case-Sensitive and should be comma-separated. e.g.```NginxModuleResponseHeaders                  Content-Length,Content-Type;```|
|*NginxModuleOtelExporterOtlpHeaders*           |                  | OPTIONAL: OTEL exporter headers like Meta data related exposrted end point. a list of key value pairs, and these are expected to be represented in a format matching to the W3C Correlation-Context, except that additional semi-colon delimited metadata is not supported, i.e.: key1=value1,key2=value2.|

### Build and Installation
#### Prerequisites
- Docker Desktop should be installed on the system

#### Platform Supported
- Supports both stable(1.22.0) and mainline(1.23.1).
- Earlier support of v1.18.0 is deprecated.
- The build is supported for **x86-64** platforms.
- OS support: **Centos6**, **Centos7, ubuntu20.04**.

#### Automatic build and Installation

We will use Docker to run the Module. First, it is to be made sure that the Docker is up and running.
Then execute the following commands -:
```
docker-compose --profile centos_nginx build
docker-compose --profile centos_nginx up
```
Alternatively, replace the value of *centos_nginx* from **'centos_nginx'** to **'centos7_nginx'** or **'ubuntu20.04_nginx'** to build in respective supported platforms.

This would start the container alongwith the the Opentelemetry Collector and Zipkin. You can check the traces on Zipkin dashboard by checking the port number of Zipkin using ```docker ps``` command. Multiple requests can be sent using the browser.

#### Manual build and Installation

The artifact can be either downloaded or built manually.

##### Generate the artifact manually
We will use Docker to build the artifact. First, it is to be made sure that the Docker is up and running.
Then execute the following commands -:
```
docker-compose --profile centos7_nginx build
docker-compose --profile centos7_nginx up
```
Next, login into the Docker container.
After going inside the container run the following commands
```
cd /otel-webserver-module
./gradlew assembleWebServerModule
```
The above command builds the aritifact and the same is located at ```/otel-webserver-module/build```.

The build file can be copied at a suitable location in User's system by running the command ```docker cp <container_name>:/otel-webserver-module/build/opentelemetry-webserver-sdk-x64-linux.tgz  <desired_location>```.

##### Download the artifact

The artifact can also be downloaded from either of below links
- [Releases](https://github.com/open-telemetry/opentelemetry-cpp-contrib/releases) - Make sure to download from releases/tags having ```webserver/vXX.XX.XX``` e.g [webserver/v1.0.0](https://github.com/open-telemetry/opentelemetry-cpp-contrib/releases/tag/webserver%2Fv1.0.0)
- [GitHub Actions](https://github.com/open-telemetry/opentelemetry-cpp-contrib/actions/workflows/webserver.yml) - Click on any top successful workflow runs on main branch and the artifact would be available for download.

##### Installation Steps

In order to install, untar the artifact to /opt directory.
```
tar -xf opentelemetry-webserver-sdk-x64-linux.tgz -C /opt
cd /opt/opentelemetry-webserver-sdk/
./install.sh
```
Copy the ```conf/nginx/opentelemetry_module.conf``` to /opt/.
Make sure to edit the directives values according to your need e.g NginxModuleOtelExporterEndpoint should point to collector url.
Edit the nginx.conf to provide the reference to opentelemetry_module.conf and shared library.
Please mind the order and location of the below entries by referring to ```conf/nginx/nginx.conf```.
```
load_module /opt/opentelemetry-webserver-sdk/WebServerModule/Nginx/<nginx-version>/ngx_http_opentelemetry_module.so;
include /opt/opentelemetry_module.conf;
```

Before running Nginx webserver, make sure to update LD_LIBRARY_PATH to pick up opentelemetry dependencies.
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/opentelemetry-webserver-sdk/sdk_lib/lib
```

### Usability of the downloaded artifact
The downloaded artifact from [Release/Tag](https://github.com/open-telemetry/opentelemetry-cpp-contrib/releases) or [GitHub Actions](https://github.com/open-telemetry/opentelemetry-cpp-contrib/actions/workflows/webserver.yml) is built on CentOS7. This contains shared libraries for both apache and nginx instrumentation. The shared libraries can be located at ```WebServerModule/Apache``` or ```WebServerModule/Nginx``` for respective webservers. But, the common libraries, related to opentelemetry, are located at ```sdk_lib/lib/``` which are used by both apache and nginx instrumentation.

Currently, artifact is generated on x86-64 is published.
**Therefore, the artifact should work on any linux distribution running on x86-64 plarform and having glibc version >= 2.17.**

### Maintainers
* [Kumar Pratyush](https://github.com/kpratyus), Cisco
* [Debajit Das](https://github.com/DebajitDas), Cisco

### Blogs
* [Instrument Apache HttpServer with OpenTelemetry](https://opentelemetry.io/blog/2022/instrument-apache-httpd-server/)

