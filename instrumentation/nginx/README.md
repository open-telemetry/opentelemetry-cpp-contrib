# OpenTelemetry nginx module

Adds OpenTelemetry distributed tracing support to nginx.

Supported propagation types:
* [W3C](https://w3c.github.io/trace-context/) - default
* [b3](https://github.com/openzipkin/b3-propagation)

## Requirements

* OS: Linux. Test suite currently runs on Ubuntu 18.04, 20.04, 20.10.
* [Nginx](http://nginx.org/en/download.html)
  * both stable (`1.18.0`) and mainline (`1.19.8`)
* Nginx modules
  * ngx_http_upstream_module (proxy_pass)
  * ngx_http_fastcgi_module (fastcgi_pass)

Additional platforms and/or versions coming soon.

## Dependencies (for building)

1. [gRPC](https://github.com/grpc/grpc) - currently the only supported exporter is OTLP. This requirement will be lifted
   once more exporters become available.
2. [opentelemetry-cpp](https://github.com/open-telemetry/opentelemetry-cpp) - opentelemetry-cpp needs to be built with
   position independent code and OTLP support, e.g.:
```
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DWITH_OTLP=ON ..
```

## Building

```
mkdir build
cd build
cmake ..
make
```

## Usage

Modify nginx.conf, or see the [example](test/conf/nginx.conf)

```
load_module /path/to/otel_ngx_module.so;

http {
  opentelemetry_config /conf/otel-nginx.toml;

  server {
    listen 80;
    server_name otel_example;

    root /var/www/html;

    location = / {
      opentelemetry_operation_name my_example_backend;
      opentelemetry_propagate;
      proxy_pass http://localhost:3500/;
    }

    location = /b3 {
      opentelemetry_operation_name my_other_backend;
      opentelemetry_propagate b3;
      # Adds a custom attribute to the span
      opentelemetry_attribute "req.time" "$msec";
      proxy_pass http://localhost:3501/;
    }

    location ~ \.php$ {
      root /var/www/html/php;
      opentelemetry_operation_name php_fpm_backend;
      opentelemetry_propagate;
      fastcgi_pass localhost:9000;
      include fastcgi.conf;
    }
  }
}

```

Example [otel-nginx.toml](test/conf/otel-nginx.toml):
```toml
exporter = "otlp"
processor = "batch"

[exporters.otlp]
# Alternatively the OTEL_EXPORTER_OTLP_ENDPOINT environment variable can also be used.
host = "localhost"
port = 4317

[processors.batch]
max_queue_size = 2048
schedule_delay_millis = 5000
max_export_batch_size = 512

[service]
name = "nginx-proxy" # Opentelemetry resource name

[sampler]
name = "AlwaysOn" # Also: AlwaysOff, TraceIdRatioBased
ratio = 0.1
parent_based = false
```

## nginx directives

### `opentelemetry`

Enable or disable OpenTelemetry (default: enabled).

- **required**: `false`
- **syntax**: `opentelemetry on|off`
- **block**: `http`, `server`, `location`

### `opentelemetry_attribute`

Adds a custom attribute to the span. It is possible to access nginx variables, e.g.
`opentelemetry_attribute "my.user.agent" "$http_user_agent"`.

- **required**: `false`
- **syntax**: `opentelemetry_attribute <key> <value>`
- **block**: `http`, `server`, `location`

### `opentelemetry_config`

Exporters, processors

- **required**: `true`
- **syntax**: `opentelemetry_config /path/to/config.toml`
- **block**: `http`

### `opentelemetry_operation_name`

Set the operation name when starting a new span.

- **required**: `false`
- **syntax**: `opentelemetry_operation_name <name>`
- **block**: `http`, `server`, `location`

### `opentelemetry_propagate`

Enable propagation of distributed tracing headers, e.g. `traceparent`. When no parent trace is given, a new trace will
be started. The default propagator is W3C.

- **required**: `false`
- **syntax**: `opentelemetry_propagate` or `opentelemetry_propagate b3`
- **block**: `http`, `server`, `location`

## OpenTelemetry attributes

List of exported attributes and their corresponding nginx variables if applicable:

- `http.status_code`
- `http.method`
- `http.target`
- `http.flavor`
- `http.host` - `Host` header value
- `http.scheme` - `$scheme`
- `http.server_name` - From the `server_name` directive

## Testing

Dependencies:
* [Elixir](https://elixir-lang.org/install.html)
* [Docker](https://docs.docker.com/engine/install/)
* [Docker Compose](https://docs.docker.com/compose/install/)

In case you don't have elixir locally installed, you can run the mix commands inside a container:

```
docker run -it --rm -v $(pwd):/otel -v /var/run/docker.sock:/var/run/docker.sock -e TEST_ROOT=$(pwd)/test -w /otel elixir:1.11-alpine sh
apk --no-cache add docker-compose docker-cli
```

```
cd test/instrumentation
mix dockerfiles .. ubuntu-20.04:mainline
docker build -t otel-nginx-test/nginx -f ../Dockerfile.ubuntu-20.04.mainline ../..
docker build -t otel-nginx-test/express-backend -f ../backend/simple_express/Dockerfile ../backend/simple_express
mix test
```

## Troubleshooting

### `otel_ngx_module.so is not binary compatible`
- Make sure your nginx is compiled with `--with-compat` (`nginx -V`). On Ubuntu 18.04 the default nginx (`1.14.0`) from apt does not have compatibility enabled. nginx provides [repositories](https://docs.nginx.com/nginx/admin-guide/installing-nginx/installing-nginx-open-source/#prebuilt_ubuntu) to install more up to date versions.
----
## Nginx Instrument Build Step by Step(Note: in Linux instance)
```
$ mkdir -p ~/software && cd ~/software 
$ mkdir cmake curl grpc nginx openssl opentelemetry-cpp opentelemetry-cpp-build otel-nginx pcre zlib
```
### OpenSSL Install
```
$ cd ~/software/openssl
$ wget http://www.openssl.org/source/openssl-1.1.1g.tar.gz
$ tar -zxf openssl-1.1.1g.tar.gz
$ cd openssl-1.1.1g
# query your arch support list
$ [optional]./Configure LIST | grep -i linux
$ ./Configure linux-x86_64 --prefix=/usr/local/openssl/
$ make
$ sudo make install

$ sudo mv /usr/bin/openssl /usr/bin/openssl.old
$ sudo ln -s /usr/local/openssl/bin/openssl /usr/bin/openssl

# check openssl installation 
$ openssl version -v
```

> Possible problem
>> Problem1: openssl: error while loading shared libraries: libcrypto.so.1.1: cannot open shared object file: No such file or directory
>>> Solution1: https://blog.csdn.net/tiven1/article/details/106592679


### PCRE Install
```
$ cd ~/software/pcre
$ wget https://sourceforge.net/projects/pcre/files/pcre/8.44/pcre-8.44.tar.gz
$ tar -zxf pcre-8.44.tar.gz
$ cd pcre-8.44
$ ./configure
$ make
$ sudo make install
```

### zlib Install
```
$ cd ~/software/zlib
$ wget http://zlib.net/zlib-1.2.11.tar.gz
$ tar -zxf zlib-1.2.11.tar.gz
$ cd zlib-1.2.11
$ ./configure
$ make
$ sudo make install
```


### CMake Install
```
$ cd ~/software/cmake
$ wget https://github.com/Kitware/CMake/archive/refs/tags/v3.19.6.tar.gz
$ tar xf v3.19.6.tar.gz
$ cd CMake-3.19.6

$ sudo apt-get update
$ sudo apt-get install -y libssl-dev
$ ./configure
$ make
```

### Nginx Install
```
$ cd ~/software/nginx
$ wget https://nginx.org/download/nginx-1.18.0.tar.gz
$ tar zxf nginx-1.18.0.tar.gz
$ cd nginx-1.18.0
$ ./configure --sbin-path=/usr/local/nginx/nginx --conf-path=/usr/local/nginx/nginx.conf --pid-path=/usr/local/nginx/nginx.pid --with-pcre=../../pcre/pcre-8.44 --with-zlib=../../zlib/zlib-1.2.11 --with-http_ssl_module --with-stream --with-openssl=../../openssl/openssl-1.1.1g --with-compat
$ make
$ sudo make install

$ sudo ln -s /usr/local/nginx/nginx /usr/bin/nginx
```

### CURL Install
```
$ cd ~/software/curl
$ wget https://curl.haxx.se/download/curl-7.65.3.tar.gz
$ tar xvfz curl-7.65.3.tar.gz
$ cd curl-7.65.3
$ ./configure --prefix=/usr/local/curl/7_65_3
$ make
$ sudo make install

# if /usr/bin/curl exist
$ sudo mv /usr/bin/curl /usr/bin/curl.old
$ sudo ln -s /usr/local/curl/7_65_3/bin/curl /usr/bin/curl
```

### grpc Install
```
$ cd ~/software/grpc
$ git clone -b v1.36.4 https://github.com/grpc/grpc
$ cd grpc
$ git submodule update --init

$ mkdir -p cmake/build
$ cd cmake/build

$ mkdir -p ../../third_party/abseil-cpp/cmake/build
$ pushd "../../third_party/abseil-cpp/cmake/build"
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ../..
$ sudo make -j4 install
$ popd

$ cmake ../.. -DgRPC_INSTALL=ON -DCMAKE_BUILD_TYPE=Release -DgRPC_BUILD_TESTS=OFF -DgRPC_ABSL_PROVIDER=package  -DgRPC_SSL_PROVIDER=package
$ sudo make -j4 install
```

### opentelemetry-cpp Install
```
$ cd ~/software/opentelemetry-cpp
$ git clone --recursive -b v0.7.0 https://github.com/open-telemetry/opentelemetry-cpp
$ git submodule update --init

$ mkdir build && cd build

# if any problems occur, you can try this command first
$ [optional]sudo chmod -R 755 /usr/local/

$ cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../../opentelemetry-cpp-build -DCMAKE_PREFIX_PATH=../../../opentelemetry-cpp-build -DBUILD_TESTING=OFF -DWITH_EXAMPLES=OFF -DWITH_OTLP=ON -DCURL_LIBRARY=/usr/local/curl/7_65_3/lib/libcurl.so.4.5.0 -DCURL_INCLUDE_DIR=/usr/local/curl/7_65_3/include ..
$ make -j2
$ sudo make install

# if any problems occur, you can try this command first
$ [optional]sudo chmod -R 755 ~/software/
```

### otel-nginx Install
```
$ cd ~/software/otel-nginx
$ git clone https://github.com/open-telemetry/opentelemetry-cpp-contrib.git
$ mkdir -p opentelemetry-cpp-contrib/build && cd opentelemetry-cpp-contrib/build
$ cmake .. -DCMAKE_PREFIX_PATH=../../../opentelemetry-cpp-build
$ make
```