# httpd (Apache) OpenTelemetry module

## Features

- Supports exporters: plain file, OTLP
- Supports batch processor
- Supports propagators: [w3c trace-context](https://www.w3.org/TR/trace-context/), [B3](https://github.com/openzipkin/b3-propagation)

## Requirements

- C++11
- [OpenTelemetry-Cpp](https://github.com/open-telemetry/opentelemetry-cpp)
- httpd (Apache) ver. 2.4.x on Linux (Current release tested only with Ubuntu LTS 18.04)
- Bazel 3.7.x

## Build + development

Build can be done within docker or alternatively check Development section for Ubuntu below.  Execute: `make build` to start build process.

After build is successful enable module for httpd (Apache) the `httpd_install_otel.sh` (prepared for Docker Image) script can be used for this.

### Development

For development purposes (to get inside docker) execute `make start` and `make devsh` for each extra terminal.

Build is done with Bazel. Execute: `./build.sh` and it should create file `otel.so` inside `bazel-out/k8-opt/bin` directory.

You can just link `/mnt/host` into `/root/src`:

```bash
cd /root
mv src src-org
ln -s /mnt/host src
```

When local changes are made, you need to restart the `httpd` server to load new version of library, to do that: `apachectl stop; ./build.sh && apachectl start`

### Development (Ubuntu)

On Ubuntu you need packages listed here: [setup_environment.sh](./setup_environment.sh) which are prerequisites to compile opentelemetry-cpp and here: [setup-buildtools.sh](./setup-buildtools.sh) for apache development stuff. Then just execute [bulid.sh](./build.sh).

### Run formatting check

Please make sure that code is well formatted with this command:

```bash
./tools/check-formatting.sh
```

### Testing

Integration tests exists in `tests` directory. Please run `run-all.sh` to check functionality.

## Configuration

At the moment only one global exporter is allowed for entire daemon. Include it following way:

```
<IfModule mod_otel.cpp>
OpenTelemetryExporter file
OpenTelemetryPath /tmp/spans
</IfModule>
```

in a master configuration which usually is in `/etc/apache2` directory.

If no `OpenTelemetryPath` is specified then spans goes to standard error output which is apache error log.

More detailed information about configuration options can be found in [provided configuration file](./opentelemetry.conf)
