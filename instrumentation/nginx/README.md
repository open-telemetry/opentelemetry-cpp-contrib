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

Download the .so file from the latest [GitHub Action run](https://github.com/open-telemetry/opentelemetry-cpp-contrib/actions/workflows/nginx.yml) or follow the instructions above to build. Then modify nginx.conf, or see the [example](test/conf/nginx.conf)

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
# Optional: enable SSL, for endpoints that support it
# use_ssl = true
# Optional: set a filesystem path to a pem file to be used for SSL encryption
# (when use_ssl = true)
# ssl_cert_path = "/path/to/cert.pem"

[processors.batch]
max_queue_size = 2048
schedule_delay_millis = 5000
max_export_batch_size = 512

[service]
# Can also be set by the OTEL_SERVICE_NAME environment variable.
name = "nginx-proxy" # Opentelemetry resource name

[sampler]
name = "AlwaysOn" # Also: AlwaysOff, TraceIdRatioBased
ratio = 0.1
parent_based = false
```

Here's what it would look like if you used the OTLP exporter, but only set the endpoint with an environment variables (e.g. `OTEL_EXPORTER_OTLP_ENDPOINT="localhost:4317"`).
```toml
exporter = "otlp"
processor = "batch"

[exporters.otlp]

[processors.batch]
max_queue_size = 2048
schedule_delay_millis = 5000
max_export_batch_size = 512

[service]
name = "nginx-proxy" # Opentelemetry resource name
```

To use other environment variables defined in the [specification](https://github.com/open-telemetry/opentelemetry-specification/blob/main/specification/protocol/exporter.md), must add the "env" directive.

```
env OTEL_EXPORTER_OTLP_HEADERS;

http {
  .
  .
  .
}
```

## nginx directives

### `opentelemetry`

Enable or disable OpenTelemetry (default: enabled).

- **required**: `false`
- **syntax**: `opentelemetry on|off`
- **block**: `http`, `server`, `location`

### `opentelemetry_trust_incoming_spans`

Enables or disables using spans from incoming requests as parent for created ones. (default: enabled).

- **required**: `false`
- **syntax**: `opentelemetry_trust_incoming_spans on|off`
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

The same inheritance rules as [`proxy_set_header`](http://nginx.org/en/docs/http/ngx_http_proxy_module.html#proxy_set_header) apply, which means this directive is applied at the current configuration level if and only if there are no `proxy_set_header` directives defined on a lower level.

- **required**: `false`
- **syntax**: `opentelemetry_propagate` or `opentelemetry_propagate b3`
- **block**: `http`, `server`, `location`

### `opentelemetry_capture_headers`

Enables the capturing of request and response headers. (default: disabled).

- **required**: `false`
- **syntax**: `opentelemetry_capture_headers on|off`
- **block**: `http`, `server`, `location`

### `opentelemetry_sensitive_header_names`

Sets the captured header value to `[REDACTED]` for all headers where the name matches the given regex (case insensitive).

- **required**: `false`
- **syntax**: `opentelemetry_sensitive_header_names <regex>`
- **block**: `http`, `server`, `location`

### `opentelemetry_sensitive_header_values`

Sets the captured header value to `[REDACTED]` for all headers where the value matches the given regex (case insensitive).

- **required**: `false`
- **syntax**: `opentelemetry_sensitive_header_values <regex>`
- **block**: `http`, `server`, `location`

### `opentelemetry_ignore_paths`

No span will be created for URIs matching the given regex (case insensitive).

- **required**: `false`
- **syntax**: `opentelemetry_ignore_paths <regex>`
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
- `http.user_agent` - `User-Agent` header value
- `http.request.header.*` - The request headers (except `Host` and `User-Agent`)
- `http.response.header.*` - The response headers
- `net.host.port` - `$server_port`
- `net.peer.ip` - `$remote_addr`
- `net.peer.port` - `$remote_port`

## nginx variables

The following nginx variables are set by the instrumentation:

- `opentelemetry_context_traceparent` - [W3C trace
  context](https://www.w3.org/TR/trace-context/#trace-context-http-headers-format), e.g.: `00-0af7651916cd43dd8448eb211c80319c-b9c7c989f97918e1-01`
- `opentelemetry_context_b3` - Trace context in the [B3
  format](https://github.com/openzipkin/b3-propagation#single-header). Only set when using `opentelemetry_propagate b3`.
- `opentelemetry_trace_id` - Trace Id of the current span
- `opentelemetry_span_id` - Span Id of the current span

This can be used to add `Server-Timing` header:

```
add_header Server-Timing "traceparent;desc=\"$opentelemetry_context_traceparent\"";
```

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
