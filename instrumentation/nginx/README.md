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
host = "localhost"
port = 4317

[processors.batch]
max_queue_size = 2048
schedule_delay_millis = 5000
max_export_batch_size = 512

[service]
name = "nginx-proxy" # Opentelemetry resource name
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

In case you don't have elixir locally installed, you can run the mix commands inside `docker run -it --rm -v $(pwd):/otel -w /otel elixir:1.11 bash`

```
cd test/instrumentation
mix dockerfiles .. ubuntu-20.04:mainline
cd ../..
docker build -t otel-nginx-test/nginx -f test/Dockerfile.ubuntu-20.04.mainline .
docker build -t otel-nginx-test/express-backend -f test/backend/simple_express/Dockerfile test/backend/simple_express
cd test/instrumentation
mix test
```

## Troubleshooting

### `otel_ngx_module.so is not binary compatible`
- Make sure your nginx is compiled with `--with-compat` (`nginx -V`). On Ubuntu 18.04 the default nginx (`1.14.0`) from apt does not have compatibility enabled. nginx provides [repositories](https://docs.nginx.com/nginx/admin-guide/installing-nginx/installing-nginx-open-source/#prebuilt_ubuntu) to install more up to date versions.
