#include <iostream>

#include <map>
#include <string>
#include <vector>

#include "opentelemetry/exporters/geneva/metrics/exporter.h"

#include "common/generate_metrics.h"
#include "common/socket_server.h"
#include "decoder/ifx_metrics_bin.h"
#include "decoder/kaitai/kaitaistream.h"
#include <gtest/gtest.h>

using namespace SOCKET_SERVER_NS;
using namespace kaitai;
using namespace opentelemetry::sdk::metrics;
using namespace opentelemetry::exporter::geneva::metrics;

std::string kUnixDomainPathUDS = "/tmp/ifx_unix_socket";
std::string kUnixDomainPathAbstractSocket = "@/tmp/ifx_unix_socket";

const std::string kNamespaceName = "test_ns";
const std::string kAccountName = "test_account";

const std::string kCustomNamespaceName = "custom_ns";
const std::string kCustomAccountName = "custom_account";


// "busy sleep" while suggesting that other threads run
// for a small amount of time
template <typename timeunit> void yield_for(timeunit duration) {
  auto start = std::chrono::high_resolution_clock::now();
  auto end = start + duration;
  do {
    std::this_thread::yield();
  } while (std::chrono::high_resolution_clock::now() < end);
}

struct TestServer {
  SocketServer &server;
  std::atomic<uint32_t> count{0};
  size_t count_counter_double = 0;
  size_t count_counter_long = 0;
  size_t count_up_down_counter_long = 0;
  size_t count_up_down_counter_double = 0;
  size_t count_histogram_long = 0;
  size_t count_custom_histogram_long = 0;

  TestServer(SocketServer &server) : server(server) {
    server.onRequest = [&](SocketServer::Connection &conn) {
      try {
        std::stringstream ss{conn.request_buffer};
        kaitai::kstream ks(&ss);
        try {
          ifx_metrics_bin_t event_bin = ifx_metrics_bin_t(&ks);

          if (event_bin.event_id() == kCounterDoubleEventId) {
            EXPECT_EQ(event_bin.event_id(), kCounterDoubleEventId);
            auto event_body = event_bin.body();
            if (static_cast<ifx_metrics_bin_t::single_double_value_t *>(
                    event_body->value_section())
                    ->value() == kCounterDoubleValue1) {
              EXPECT_EQ(event_body->num_dimensions(),
                        kCounterDoubleCountDimensions);
              EXPECT_EQ(event_body->dimensions_values()->at(0)->value(),
                        kCounterDoubleAttributeValue1);
              EXPECT_EQ(event_body->dimensions_names()->at(0)->value(),
                        kCounterDoubleAttributeKey1);
              EXPECT_EQ(event_body->metric_name()->value(),
                        kCounterDoubleInstrumentName);
              count_counter_double++;
            }
            if (static_cast<ifx_metrics_bin_t::single_double_value_t *>(
                    event_body->value_section())
                    ->value() == kCounterDoubleValue2) {
              EXPECT_EQ(event_body->num_dimensions(),
                        kCounterDoubleCountDimensions + 1);
              EXPECT_EQ(event_body->dimensions_values()->at(0)->value(),
                        kCounterDoubleAttributeValue2);
              EXPECT_EQ(event_body->dimensions_names()->at(0)->value(),
                        kCounterDoubleAttributeKey2);
              EXPECT_EQ(event_body->dimensions_values()->at(1)->value(),
                        kCounterDoubleAttributeValue3);
              EXPECT_EQ(event_body->dimensions_names()->at(1)->value(),
                        kCounterDoubleAttributeKey3);
              EXPECT_EQ(event_body->metric_name()->value(),
                        kCounterDoubleInstrumentName);
              count_counter_double++;
            }
            if (static_cast<ifx_metrics_bin_t::single_double_value_t *>(
                    event_body->value_section())
                    ->value() == kUpDownCounterLongValue) {
              EXPECT_EQ(event_body->num_dimensions(),
                        kUpDownCounterLongCountDimensions);
              EXPECT_EQ(event_body->dimensions_values()->at(0)->value(),
                        kUpDownCounterLongAttributeValue1);
              EXPECT_EQ(event_body->dimensions_names()->at(0)->value(),
                        kUpDownCounterLongAttributeKey1);
              count_up_down_counter_long++;
            }
            if (static_cast<ifx_metrics_bin_t::single_double_value_t *>(
                    event_body->value_section())
                    ->value() == kUpDownCounterDoubleValue) {
              EXPECT_EQ(event_body->num_dimensions(),
                        kUpDownCounterDoubleCountDimensions);
              EXPECT_EQ(event_body->dimensions_values()->at(0)->value(),
                        kUpDownCounterDoubleAttributeValue1);
              EXPECT_EQ(event_body->dimensions_names()->at(0)->value(),
                        kUpDownCounterDoubleAttributeKey1);
              count_up_down_counter_double++;
            }
            EXPECT_EQ(event_body->metric_account()->value(), kAccountName);
            EXPECT_EQ(event_body->metric_namespace()->value(), kNamespaceName);
          } else if (event_bin.event_id() == kCounterLongEventId) {
            EXPECT_EQ(event_bin.event_id(), kCounterLongEventId);
            auto event_body = event_bin.body();
            if (static_cast<ifx_metrics_bin_t::single_uint64_value_t *>(
                  event_body->value_section())->value() == kCounterLongValue) {
              EXPECT_EQ(static_cast<ifx_metrics_bin_t::single_uint64_value_t *>(
                          event_body->value_section())
                          ->value(),
                      kCounterLongValue);
              EXPECT_EQ(event_body->num_dimensions(),
                      kCounterLongCountDimensions);
              EXPECT_EQ(event_body->dimensions_values()->at(0)->value(),
                      kCounterLongAttributeValue1);
              EXPECT_EQ(event_body->dimensions_names()->at(0)->value(),
                      kCounterLongAttributeKey1);
              count_counter_long++;
              EXPECT_EQ(event_body->metric_account()->value(), kAccountName);
              EXPECT_EQ(event_body->metric_namespace()->value(), kNamespaceName);
            } else if (static_cast<ifx_metrics_bin_t::single_uint64_value_t *>(
                event_body->value_section())->value() == kCounterCustomLongValue) {
              EXPECT_EQ(static_cast<ifx_metrics_bin_t::single_uint64_value_t *>(
                          event_body->value_section())
                          ->value(),
                      kCounterCustomLongValue);
              EXPECT_EQ(event_body->num_dimensions(),
                      kCounterLongCountDimensions);
              EXPECT_EQ(event_body->dimensions_values()->at(0)->value(),
                      kCounterLongAttributeValue1);
              EXPECT_EQ(event_body->dimensions_names()->at(0)->value(),
                      kCounterLongAttributeKey1);
              count_counter_long++;
              EXPECT_EQ(event_body->metric_account()->value(), kCustomAccountName);
              EXPECT_EQ(event_body->metric_namespace()->value(), kCustomNamespaceName);
            }
          }  
           else if (event_bin.event_id() == kHistogramLongEventId) {
            EXPECT_EQ(event_bin.event_id(), kHistogramLongEventId);
            auto event_body = event_bin.body();
            EXPECT_EQ(event_body->num_dimensions(),
                      kCounterLongCountDimensions);
            EXPECT_EQ(event_body->dimensions_values()->at(0)->value(),
                      kHistogramLongAttributeValue1);
            EXPECT_EQ(event_body->dimensions_names()->at(0)->value(),
                      kHistogramLongAttributeKey1);
            if (static_cast<ifx_metrics_bin_t::ext_aggregated_uint64_value_t *>(
                    event_body->value_section())
                    ->sum() == kHistogramLongSum) {
              EXPECT_EQ(event_body->metric_account()->value(), kAccountName);
              EXPECT_EQ(event_body->metric_namespace()->value(), kNamespaceName);
              count_histogram_long++;
            } else if (static_cast<ifx_metrics_bin_t::ext_aggregated_uint64_value_t *>(
                    event_body->value_section())
                    ->sum() == kHistogramCustomLongSum) {
              EXPECT_EQ(event_body->metric_account()->value(), kCustomAccountName);
              EXPECT_EQ(event_body->metric_namespace()->value(), kCustomNamespaceName);
              count_custom_histogram_long++;
            }
            EXPECT_EQ(
                static_cast<ifx_metrics_bin_t::ext_aggregated_uint64_value_t *>(
                    event_body->value_section())
                    ->min(),
                kHistogramLongMin);
            EXPECT_EQ(
                static_cast<ifx_metrics_bin_t::ext_aggregated_uint64_value_t *>(
                    event_body->value_section())
                    ->max(),
                kHistogramLongMax);
            EXPECT_EQ(
                static_cast<ifx_metrics_bin_t::histogram_value_count_pairs_t *>(
                    event_body->histogram()->body())
                    ->distribution_size(),
                kHistogramLongNonEmptyBucketSize);

            size_t index_all_buckets = 0;
            size_t index_nonempty_buckets = 0;
            for (auto value : kHistogramLongBoundaries) {
              if (kHistogramLongCounts[index_all_buckets] > 0) {
                EXPECT_EQ(
                    static_cast<ifx_metrics_bin_t::pair_value_count_t *>(
                        static_cast<
                            ifx_metrics_bin_t::histogram_value_count_pairs_t *>(
                            event_body->histogram()->body())
                            ->columns()
                            ->at(index_nonempty_buckets))
                        ->count(),
                    kHistogramLongCounts[index_all_buckets]);
                EXPECT_EQ(
                    static_cast<ifx_metrics_bin_t::pair_value_count_t *>(
                        static_cast<
                            ifx_metrics_bin_t::histogram_value_count_pairs_t *>(
                            event_body->histogram()->body())
                            ->columns()
                            ->at(index_nonempty_buckets))
                        ->value(),
                    value);
                index_nonempty_buckets++;
              }
              index_all_buckets++;
            }
          }

        } catch (...) {
          EXPECT_NE("READ FAILED", "READ FAILED");
        }
        conn.state.insert(SocketServer::Connection::Responding);
        conn.request_buffer.clear();
      } catch (std::exception &) {
        conn.state.insert(SocketServer::Connection::Receiving);
        // skip invalid payload
      }
    };
  }

  void Start() { server.Start(); }

  void Stop() { server.Stop(); }

  void WaitForEvents(uint32_t expectedCount, uint32_t timeout) {
    if (count.load() != expectedCount) {
      yield_for(std::chrono::milliseconds(timeout));
    }
    EXPECT_EQ(count.load(), expectedCount);
  }
};

class GenericMetricsExporterTextFixture: public ::testing::TestWithParam<std::string>
{};

TEST_P(GenericMetricsExporterTextFixture, BasicTests) {

  std::string kUnixDomainPath = GetParam();
  bool isRunning = true;

  // Start test server
  SocketAddr destination(kUnixDomainPath.data(), true);
  SocketParams params{AF_UNIX, SOCK_STREAM, 0};
  SocketServer socketServer(destination, params);
  TestServer testServer(socketServer);
  testServer.Start();
  yield_for(std::chrono::milliseconds(500));

  // conn_string:
  // `Endpoint=unix:{udsPath};Account={MetricAccount};Namespace={MetricNamespace}`
  std::string conn_string = "Endpoint=unix://" + kUnixDomainPath +
                            ";Account=" + kAccountName +
                            ";Namespace=" + kNamespaceName;
  ExporterOptions options{conn_string};
  opentelemetry::exporter::geneva::metrics::Exporter exporter(options);


  // export sum aggregation - double
  auto metric_data = GenerateSumDataDoubleMetrics();
  exporter.Export(metric_data);
  yield_for(std::chrono::milliseconds(1000));

  // export sum aggregation - long
  metric_data = GenerateSumDataLongMetrics();
  exporter.Export(metric_data);
  yield_for(std::chrono::milliseconds(1000));

  // export sum aggregation - long, custom account & ns
  metric_data = GenerateSumDataLongMetrics(kCustomAccountName, kCustomNamespaceName);
  exporter.Export(metric_data);
  yield_for(std::chrono::milliseconds(1000));

  // export sum aggregation - long - non monotonic
  metric_data = GenerateSumDataLongMetricsNonMonotonic();
  exporter.Export(metric_data);
  yield_for(std::chrono::milliseconds(1000));

  // export sum aggregation - double - non monotonic
  metric_data = GenerateSumDataDoubleMetricsNonMonotonic();
  exporter.Export(metric_data);
  yield_for(std::chrono::milliseconds(1000));

  // export histogram aggregation - long
  metric_data = GenerateHistogramDataLongMetrics();
  exporter.Export(metric_data);
  yield_for(std::chrono::milliseconds(1000));

  // export histogram aggregation - long, custom account & ns
  metric_data = GenerateHistogramDataLongMetrics(kCustomAccountName, kCustomNamespaceName);
  exporter.Export(metric_data);
  yield_for(std::chrono::milliseconds(1000));

  EXPECT_EQ(testServer.count_counter_double, 1);
  EXPECT_EQ(testServer.count_counter_long, 2);
  EXPECT_EQ(testServer.count_up_down_counter_long, 1);
  EXPECT_EQ(testServer.count_up_down_counter_double, 1);
  EXPECT_EQ(testServer.count_histogram_long, 1);
  EXPECT_EQ(testServer.count_custom_histogram_long, 1);

  testServer.Stop();
}

INSTANTIATE_TEST_SUITE_P(GenericMetricsExporterText, GenericMetricsExporterTextFixture,
    ::testing::Values(kUnixDomainPathUDS, kUnixDomainPathAbstractSocket));


