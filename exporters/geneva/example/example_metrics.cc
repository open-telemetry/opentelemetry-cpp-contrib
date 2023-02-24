// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/geneva/metrics/exporter.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/sdk/metrics/aggregation/default_aggregation.h"
#include "opentelemetry/sdk/metrics/aggregation/histogram_aggregation.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h"
#include "opentelemetry/sdk/metrics/meter.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"
#include <memory>
#include <thread>

#include "foo_library.h"

namespace metric_sdk = opentelemetry::sdk::metrics;
namespace nostd = opentelemetry::nostd;
namespace common = opentelemetry::common;
namespace geneva_exporter = opentelemetry::exporter::geneva::metrics;
namespace metrics_api = opentelemetry::metrics;

namespace {

const std::string kUnixDomainPath = "/tmp/ifx_unix_socket";
const std::string kNamespaceName = "test_ns";

void initMetrics(const std::string &name, const std::string &account_name) {

  std::string conn_string =
      "Account=" + account_name + ";Namespace=" + kNamespaceName;
#ifndef _WIN32
  conn_string = "Endpoint=unix://" + kUnixDomainPath + ";" + conn_string;
#endif

  geneva_exporter::ExporterOptions options{conn_string};
  std::unique_ptr<metric_sdk::PushMetricExporter> exporter{
      new geneva_exporter::Exporter(options)};

  std::string version{"1.2.0"};
  std::string schema{"https://opentelemetry.io/schemas/1.2.0"};

  // Initialize and set the global MeterProvider
  metric_sdk::PeriodicExportingMetricReaderOptions reader_options;
  reader_options.export_interval_millis = std::chrono::milliseconds(1000);
  reader_options.export_timeout_millis = std::chrono::milliseconds(500);
  std::unique_ptr<metric_sdk::MetricReader> reader{
      new metric_sdk::PeriodicExportingMetricReader(std::move(exporter),
                                                    reader_options)};
  auto provider = std::shared_ptr<metrics_api::MeterProvider>(
      new metric_sdk::MeterProvider());
  auto p = std::static_pointer_cast<metric_sdk::MeterProvider>(provider);
  p->AddMetricReader(std::move(reader));

  // counter view
  std::string counter_name = name + "_counter";
  std::unique_ptr<metric_sdk::InstrumentSelector> instrument_selector{
      new metric_sdk::InstrumentSelector(metric_sdk::InstrumentType::kCounter,
                                         counter_name)};
  std::unique_ptr<metric_sdk::MeterSelector> meter_selector{
      new metric_sdk::MeterSelector(name, version, schema)};
  std::unique_ptr<metric_sdk::View> sum_view{new metric_sdk::View{
      name, "description", metric_sdk::AggregationType::kSum}};
  p->AddView(std::move(instrument_selector), std::move(meter_selector),
             std::move(sum_view));

  // observable counter view
  std::string observable_counter_name = name + "_observable_counter";
  std::unique_ptr<metric_sdk::InstrumentSelector>
      observable_instrument_selector{new metric_sdk::InstrumentSelector(
          metric_sdk::InstrumentType::kObservableCounter,
          observable_counter_name)};
  std::unique_ptr<metric_sdk::MeterSelector> observable_meter_selector{
      new metric_sdk::MeterSelector(name, version, schema)};
  std::unique_ptr<metric_sdk::View> observable_sum_view{new metric_sdk::View{
      name, "description", metric_sdk::AggregationType::kSum}};
  p->AddView(std::move(observable_instrument_selector),
             std::move(observable_meter_selector),
             std::move(observable_sum_view));

  // histogram view
  std::string histogram_name = name + "_histogram";
  std::unique_ptr<metric_sdk::InstrumentSelector> histogram_instrument_selector{
      new metric_sdk::InstrumentSelector(metric_sdk::InstrumentType::kHistogram,
                                         histogram_name)};
  std::unique_ptr<metric_sdk::MeterSelector> histogram_meter_selector{
      new metric_sdk::MeterSelector(name, version, schema)};
  std::shared_ptr<opentelemetry::sdk::metrics::AggregationConfig>
      aggregation_config{
          new opentelemetry::sdk::metrics::HistogramAggregationConfig()};
  static_cast<opentelemetry::sdk::metrics::HistogramAggregationConfig *>(
      aggregation_config.get())
      ->boundaries_ = std::vector<double>{0.0,   50.0,   100.0,  250.0,  500.0,
                                        750.0, 1000.0, 2500.0, 5000.0, 10000.0};
  std::unique_ptr<metric_sdk::View> histogram_view{new metric_sdk::View{
      name, "description", metric_sdk::AggregationType::kHistogram,
      aggregation_config}};
  p->AddView(std::move(histogram_instrument_selector),
             std::move(histogram_meter_selector), std::move(histogram_view));
  metrics_api::Provider::SetMeterProvider(provider);
}
} // namespace

int main(int argc, char **argv) {
  std::string account_name = "TestAccount";
  std::string example_type;
  if (argc >= 2) {
    account_name = argv[1];
    if (argc >= 3) {
      example_type = argv[2];
    }
  }

  std::string name{"ostream_metric_example"};
  initMetrics(name, account_name);

  if (example_type == "counter") {
    FooLibrary::counter_example(name);
  } else if (example_type == "observable_counter") {
    FooLibrary::observable_counter_example(name);
  } else if (example_type == "histogram") {
    FooLibrary::histogram_example(name);
  } else {
    std::thread counter_example{&FooLibrary::counter_example, name};
    std::thread observable_counter_example{
        &FooLibrary::observable_counter_example, name};
    std::thread histogram_example{&FooLibrary::histogram_example, name};

    counter_example.join();
    observable_counter_example.join();
    histogram_example.join();
  }
}
