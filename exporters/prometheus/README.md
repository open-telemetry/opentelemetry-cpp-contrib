# Prometheus Push Exporter for OpenTelemetry C++

## Installation

### CMake Install Instructions

+ Install opentelemetry-cpp with stable metric and prometheus exporter

```bash
mkdir build_jobs
cd build_jobs
cmake -DCMAKE_PREFIX_PATH=<Where to find opentelemetry-cpp and prometheus> ..

cmake --build . -j
```

### Bazel Install Instructions

`bazel build --copt=-DENABLE_TEST --@io_opentelemetry_cpp//api:with_abseil //...`

## Usage

Install the exporter on your application and pass the options.

```cpp
#include <opentelemetry/exporters/prometheus/prometheus_push_exporter.h>

opentelemetry::exporter::metrics::PrometheusPushExporterOptions options;
options.host = "localhost";
options.port = "80";
options.jobname = "jobname";

auto exporter = std::unique_ptr<opentelemetry::sdk::metrics::MetricExporter>(
    new opentelemetry::exporter::metrics::PrometheusPushExporter(options));

opentelemetry::sdk::metrics::PeriodicExportingMetricReaderOptions reader_options;
auto reader = std::unique_ptr<opentelemetry::sdk::metrics::MetricReader>(
    new opentelemetry::sdk::metrics::PeriodicExportingMetricReader(std::move(exporter), options));

auto provider = nostd::shared_ptr<opentelemetry::metrics::MeterProvider>(
    new opentelemetry::sdk::metrics::MeterProvider());
static_cast<opentelemetry::sdk::metrics::MeterProvider *>(provider.get())->AddMetricReader(std::move(reader));

// Set the global tracer provider
opentelemetry::metrics::Provider::SetMeterProvider(provider);
```
