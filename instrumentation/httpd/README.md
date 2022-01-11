# httpd (Apache) OpenTelemetry module

## Features

- Supports exporters: plain file, OTLP
- Supports batch processor
- Supports propagators: [w3c trace-context](https://www.w3.org/TR/trace-context/), [B3](https://github.com/openzipkin/b3-propagation)

## Requirements

- httpd (Apache) ver. 2.4.x on Linux (Current release tested only with Ubuntu LTS 18.04 & 20.04)

### Usage

For manual build please check below. Otherwise please use one of the [released versions](/../releases).

### From Docker Image

To start using mod_otel you can add it following way:
```Dockerfile
FROM httpd
ADD https://github.com/open-telemetry/opentelemetry-cpp-contrib/releases/download/httpd%2Fv0.1.0/ubuntu-20.04_mod-otel.so.zip /tmp
ADD https://raw.githubusercontent.com/open-telemetry/opentelemetry-cpp-contrib/main/instrumentation/httpd/opentelemetry.conf /usr/local/apache2/conf/extra/

RUN mv /tmp/ubuntu-20.04_mod-otel.so.zip /usr/local/apache2/modules/mod_otel.so.gz
RUN gzip -d /usr/local/apache2/modules/mod_otel.so.gz

RUN echo "LoadFile /usr/lib/x86_64-linux-gnu/libstdc++.so.6" >> /usr/local/apache2/conf/httpd.conf
RUN echo "LoadModule otel_module modules/mod_otel.so" >> /usr/local/apache2/conf/httpd.conf
RUN echo "Include conf/extra/opentelemetry.conf" >> /usr/local/apache2/conf/httpd.conf
```

### Logging traces

Once module is enabled you have access to currently processed span via environment variables. Those can be accessed under:
- `%{OTEL_SPANID}e` for SpanID
- `%{OTEL_TRACEID}e` for TraceID
- `%{OTEL_TRACEFLAGS}e` for TraceID
- `%{OTEL_TRACESTATE}e` for TraceState
when using [LogFormat directive](https://httpd.apache.org/docs/2.4/mod/mod_log_config.html#formats).

### Installation

Mod_otel works as a module which is loaded when Apache starts. It is written in C++ therefore standard library has to be included as well. Below is an example of lines which should be added to your configuration file (usually `/etc/httpd/conf.d` or equivalent):

```
LoadFile /usr/lib/x86_64-linux-gnu/libstdc++.so.6
LoadModule otel_module $SCRIPT_DIR/bazel-out/k8-opt/bin/otel.so
```

Please check that module is loaded correctly with command `apache2ctl configtest`

### Configuration

Below is the list of possible OpenTelemetry directives.

__OpenTelemetryExporter__
Configures exporter to be used. At the moment only one global exporter is allowed for entire daemon. Possible values for this setting are:
- `file` - to export to a file
- `otlp` - to export using the OpenTelemetry Protocol

__OpenTelemetryPath__
This option specifies file to which to export when `file` exporter is used. If no `OpenTelemetryPath` is specified then spans goes to standard error output which is apache error log.

__OpenTelemetryEndpoint__
OpenTelemetryEndpoint specifies where to export spans when OTLP is used. Put hostname and then port. Example value: `host.docker.internal:4317`

__OpenTelemetryBatch__
This directive takes 3 numerical arguments for batch processing:
- Max Queue Size
- Delay (in milliseconds, 1000 = 1s)
- Max Export Batch Size
- 
For example `OpenTelemetryBatch 10 5000 5`

__OpenTelemetryPropagators__
OpenTelemetryPropagators sets which context propagator should be used (defaults to none). Currently supported values are:
- `trace-context`
- `b3`
- `b3-multiheader`

__OpenTelemetryIgnoreInbound__
 OpenTelemetryIgnoreInbound indicates that we don't trust incoming context. This is a safe default when httpd is an edge server with traffic from Internet. Set it to false only if you run httpd in safe/trusted environment. Possbile values are `on` and `off` (defaults to on).
 
 __OpenTelemetrySetAttribute__
Allows to add extra attribute for each span. It takes two text arguments. For example `OpenTelemetrySetAttribute foo bar` 
 
List of configuration options can be found in [provided configuration file](./opentelemetry.conf)

## Development

### Requirements

- C++11
- [OpenTelemetry-Cpp](https://github.com/open-telemetry/opentelemetry-cpp)
- Bazel 3.7.x

### Build
Build can be done within docker or alternatively check Development section for Ubuntu below.  Execute: `make build` to start build process.

After build is successful enable module for httpd (Apache) the `httpd_install_otel.sh` (prepared for Docker Image) script can be used for this.

For development purposes (to get inside docker) execute `make start` and `make devsh` for each extra terminal.

Build is done with Bazel. Execute: `./build.sh` and it should create file `otel.so` inside `bazel-out/k8-opt/bin` directory.

You can just link `/mnt/host` into `/root/src`:

```bash
cd /root
mv src src-org
ln -s /mnt/host src
```

When local changes are made, you need to restart the `httpd` server to load new version of library, to do that: `apachectl stop; ./build.sh && apachectl start`

### Prerequisites (Ubuntu)

On Ubuntu you need packages listed here: [setup-environment.sh](./setup-environment.sh) which are prerequisites to compile opentelemetry-cpp and here: [setup-buildtools.sh](./setup-buildtools.sh) for apache development stuff. Then just execute [bulid.sh](./build.sh).

### Run formatting check

Please make sure that code is well formatted with this command:

```bash
./tools/check-formatting.sh
```

when contributing.

### Testing

Integration tests exists in `tests` directory. Please run `run-all.sh` to check functionality.

