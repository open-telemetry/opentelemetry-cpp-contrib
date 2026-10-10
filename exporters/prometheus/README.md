# Prometheus Exporters for OpenTelemetry C++

This module provides two Prometheus metric exporters:

- **Push exporter** — pushes collected metrics to a Prometheus
  [Pushgateway](https://github.com/prometheus/pushgateway).
- **File exporter** — serializes metrics to rotating files in the Prometheus text
  exposition format (useful when no scrape/push endpoint is available).

## Installation

### CMake Install Instructions

+ Install prometheus-cpp with both pull and push support before building
  opentelemetry-cpp (for example,
  `vcpkg install "prometheus-cpp[pull,push]" --classic`). The prometheus-cpp
  dependency built by opentelemetry-cpp 1.29 itself has push support disabled and
  cannot be used by the push exporter.
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
bazel build //...
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

The file exporter writes metrics in the
[Prometheus text exposition format](https://prometheus.io/docs/instrumenting/exposition_formats/).
It applies the same Prometheus translation as the push exporter, converting
OpenTelemetry metric names and attribute keys to Prometheus metric names and
labels. For example, `service.rusage.memory.maxrss` becomes
`service_rusage_memory_maxrss`, and `deployment.environment.name` becomes
`deployment_environment_name`.

The main difference from the OpenTelemetry C++ OTLP file metric exporter is the
output structure: the Prometheus file exporter writes Prometheus-formatted text,
while the OTLP file metric exporter writes JSON Lines (JSONL) using the OTLP
structure.

Some observability systems provide file collectors that read metrics in the
Prometheus format. This file exporter can be used for debugging or with those
collectors.

OpenTelemetry Resource attributes shared by multiple metric records can also be
exported as labels on a special `target` metadata metric, serialized as a
`target_info` sample with a value of `1`. This is controlled by
`PrometheusFileExporterOptions::populate_target_info`, which defaults to `true`.
Set it to `false` to omit this metadata metric. Resource attribute keys undergo
the same Prometheus name conversion described above.

For example, an exported file can contain the following gauge samples (excerpt):

```text
# HELP target Target metadata
# TYPE target gauge
target_info{otel_scope_name="service_coroutine",otel_scope_version="0.11.0.202610091452",telemetry_sdk_name="opentelemetry",telemetry_sdk_version="1.28.0",telemetry_sdk_language="cpp",k8s_cluster_name="local",service_instance_id="lobbysvr_1.1.12.1",deployment_environment_name="production",service_name="lobbysvr",service_version="0.11.0.202610091452",process_pid="13968"} 1
# TYPE service_rusage_memory_maxrss gauge
service_rusage_memory_maxrss{atfw_telemetry_group="default",deployment_environment_name="production",host_name="11bd2f297d9ed5141ec6189da9b42a7f8bac80a2399e4cca455b851e426772ed",service_area_zone_id="1",otel_scope_name="service_rusage",otel_scope_version="0.11.0.202610091452"} 80552
# TYPE service_rusage_cpu_all_percent gauge
service_rusage_cpu_all_percent{atfw_telemetry_group="default",deployment_environment_name="production",host_name="11bd2f297d9ed5141ec6189da9b42a7f8bac80a2399e4cca455b851e426772ed",service_area_zone_id="1",otel_scope_name="service_rusage",otel_scope_version="0.11.0.202610091452"} 0.6511
```


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
