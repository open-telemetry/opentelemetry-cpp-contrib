# Prometheus Exporters for OpenTelemetry C++

This module provides two Prometheus metric exporters:

- **Push exporter** — pushes collected metrics to a Prometheus
  [Pushgateway](https://github.com/prometheus/pushgateway).
- **File exporter** — serializes metrics to rotating files in the Prometheus text
  exposition format (useful when no scrape/push endpoint is available).

## Installation

### CMake Install Instructions

+ Install opentelemetry-cpp with the stable metrics API and the prometheus exporter.

```bash
mkdir build_jobs
cd build_jobs
cmake -DCMAKE_PREFIX_PATH=<Where to find opentelemetry-cpp and prometheus-cpp> ..

cmake --build . -j
```

This builds two libraries: `opentelemetry_prometheus_push_exporter` and
`opentelemetry_prometheus_file_exporter`.

### Bazel Install Instructions

```bash
bazel build --copt=-DENABLE_TEST //...
```

## Usage

Both exporters implement `opentelemetry::sdk::metrics::PushMetricExporter`, so they are
wired into a `PeriodicExportingMetricReader` and a `MeterProvider` the same way.

### Push exporter

```cpp
#include "opentelemetry/exporters/prometheus/push_exporter_factory.h"
#include "opentelemetry/exporters/prometheus/push_exporter_options.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_options.h"
#include "opentelemetry/sdk/metrics/meter_context_factory.h"
#include "opentelemetry/sdk/metrics/meter_provider_factory.h"
#include "opentelemetry/sdk/metrics/provider.h"

namespace metrics_sdk = opentelemetry::sdk::metrics;
namespace metrics_api = opentelemetry::metrics;
namespace prometheus  = opentelemetry::exporter::metrics;

prometheus::PrometheusPushExporterOptions options;
options.host    = "localhost";
options.port    = "9091";
options.jobname = "example_job";

auto exporter = prometheus::PrometheusPushExporterFactory::Create(options);

metrics_sdk::PeriodicExportingMetricReaderOptions reader_options;
auto reader = metrics_sdk::PeriodicExportingMetricReaderFactory::Create(std::move(exporter),
                                                                        reader_options);

auto context = metrics_sdk::MeterContextFactory::Create();
context->AddMetricReader(std::move(reader));

auto provider = metrics_sdk::MeterProviderFactory::Create(std::move(context));
std::shared_ptr<metrics_api::MeterProvider> api_provider(std::move(provider));

metrics_sdk::Provider::SetMeterProvider(api_provider);
```

### File exporter

Replace the push headers/options above with the file exporter and keep the same
reader/provider setup:

```cpp
#include "opentelemetry/exporters/prometheus/file_exporter_factory.h"
#include "opentelemetry/exporters/prometheus/file_exporter_options.h"

namespace prometheus = opentelemetry::exporter::metrics;

prometheus::PrometheusFileExporterOptions options;
options.file_pattern  = "%Y-%m-%d.prometheus.%N.log";  // rotated files
options.alias_pattern = "%Y-%m-%d.prometheus.log";     // stable alias (hard link)
options.file_size     = 20 * 1024 * 1024;              // rotate when a file reaches 20 MiB
options.rotate_size   = 3;                             // number of rotated files to keep

auto exporter = prometheus::PrometheusFileExporterFactory::Create(options);

// Wire `exporter` into a PeriodicExportingMetricReader and MeterProvider exactly as
// shown for the push exporter above.
```

`file_pattern` and `alias_pattern` accept the following placeholders:

| Placeholder        | Meaning                                 |
| ------------------ | --------------------------------------- |
| `%Y` / `%y`        | year (4 digits / last 2 digits)         |
| `%m` / `%d` / `%j` | month / day-of-month / day-of-year      |
| `%w`               | weekday (0 = Sunday)                    |
| `%H` / `%I`        | hour (24-hour / 12-hour)                |
| `%M` / `%S`        | minute / second                         |
| `%F` / `%T` / `%R` | `%Y-%m-%d` / `%H:%M:%S` / `%H:%M`       |
| `%N` / `%n`        | rotate index (starting from 0 / from 1) |
