# fluentd Exporter for OpenTelemetry C++

## Prerequisite

* [Get fluentd](https://fluentd.io/pages/quickstart.html)

## Installation

### CMake Install Instructions

Refer to install instructions [INSTALL.md](../../INSTALL.md#building-as-standalone-cmake-project).
Modify step 2 to create `cmake` build configuration for compiling fluentd as below:

```console
   $ cmake ..
   -- The C compiler identification is GNU 9.3.0
   -- The CXX compiler identification is GNU 9.3.0
   ...
   -- Configuring done
   -- Generating done
   -- Build files have been written to: /home/<user>/source/opentelemetry-cpp/build
   $
```

### VCPKG Integration

If integrating with VCPKG, make sure the cmake invocation defines the additional CMAKE_TOOLCHAIN_FILE.

For example:

```console
    $ .../opentelemetry-cpp-contrib2/exporters/fluentd$ ~/vcpkg/vcpkg install
    $ .../opentelemetry-cpp-contrib2/exporters/fluentd$ cmake -D CMAKE_TOOLCHAIN_FILE=/home/niande/vcpkg/scripts/buildsystems/vcpkg.cmake
    $ .../opentelemetry-cpp-contrib2/exporters/fluentd$ make
```

### Bazel Install Instructions

TODO

## Usage

Install the exporter on your application and pass the options. `service_name`
is an optional string. If omitted, the exporter will first try to get the
service name from the Resource. If no service name can be detected on the
Resource, a fallback name of "unknown_service" will be used.

```cpp
opentelemetry::exporter::fluentd::fluentdExporterOptions options;
options.endpoint = "http://localhost:9411/api/v2/spans";
options.service_name = "my_service";

auto exporter = std::unique_ptr<opentelemetry::sdk::trace::SpanExporter>(
    new opentelemetry::exporter::fluentd::fluentdExporter(options));
auto processor = std::shared_ptr<sdktrace::SpanProcessor>(
    new sdktrace::SimpleSpanProcessor(std::move(exporter)));
auto provider = nostd::shared_ptr<opentelemetry::trace::TracerProvider>(
    new sdktrace::TracerProvider(processor, opentelemetry::sdk::resource::Resource::Create({}),
                std::make_shared<opentelemetry::sdk::trace::AlwaysOnSampler>()));

// Set the global tracer provider
opentelemetry::trace::Provider::SetTracerProvider(provider);
```

## Viewing your traces

Please visit the fluentd UI endpoint <http://localhost:9411>
