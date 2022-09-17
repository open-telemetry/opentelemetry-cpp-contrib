#include <iostream>

#include <map>
#include <string>
#include <vector>

#include "opentelemetry/exporters/geneva/metrics/exporter.h"

#include "common/socket_server.h"
#include "common/generate_metrics.h"
#include "decoder/ifx_metrics_bin.h"
#include "decoder/kaitai/kaitaistream.h"
#include <gtest/gtest.h>

using namespace SOCKET_SERVER_NS;
using namespace kaitai;
using namespace opentelemetry::sdk::metrics;
using namespace opentelemetry::exporter::geneva::metrics;

const std::string kUnixDomainPath = "/tmp/ifx_unix_socket";
const std::string kNamespaceName = "test_ns";
const std::string kAccountName = "test_account";

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

  TestServer(SocketServer &server) : server(server) {
    server.onRequest = [&](SocketServer::Connection &conn) {        
      try {
        std::cout << "------>RECEIVED:" << conn.request_buffer.size() << "\n";
        std::stringstream ss{conn.request_buffer};
        std::cout << "2\n";
        kaitai::kstream ks(&ss);
        std::cout << "3\n";
        try {
          ifx_metrics_bin_t event_bin = ifx_metrics_bin_t(&ks);
          EXPECT_EQ(event_bin.event_id(), kCounterDoubleEventId);
          auto event_body = event_bin.body();
          EXPECT_EQ(event_body->dimensions_values()->at(0)->value(), kCounterDoubleAttributeValue1);
          EXPECT_EQ(event_body->dimensions_names()->at(0)->value(), kCounterDoubleAttributeKey1);
          EXPECT_EQ(event_body->num_dimensions(), kCounterDoubleCountDimensions);
          EXPECT_EQ(event_body->metric_account()->value(), kAccountName);
          EXPECT_EQ(event_body->metric_namespace()->value(), kNamespaceName);
          EXPECT_EQ(event_body->metric_name()->value(), kCounterDoubleInstrumentName);
         // EXPECT_EQ(event_body->value_section(), 10.0);
          std::cout << "------------>VALUE: " << event_body->value_section();

          std::cout << "4\n";
          std::cout << "\n----- EventID: " << event_bin.event_id()   << "\n";
        } catch (...)
        {
          std::cout << "read failed\n";
        }


        /*auto j = nlohmann::json::from_msgpack(msg);
        std::cout << "[" << count.fetch_add(1)
                  << "] SocketServer received payload: " << std::endl
                  << j.dump(2) << std::endl;

        conn.response_buffer = j.dump(2);*/
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


TEST(GenevaMetricsExporter, BasicTests) 
{
  bool isRunning = true;

  // Start test server
  SocketAddr destination(kUnixDomainPath.data(), true);
  SocketParams params{AF_UNIX, SOCK_STREAM, 0};
  SocketServer socketServer(destination, params);
  TestServer testServer(socketServer);
  testServer.Start();
  yield_for(std::chrono::milliseconds(500));

  // conn_string: `Endpoint=unix:{udsPath};Account={MetricAccount};Namespace={MetricNamespace}`
  std::string conn_string = "Endpoint=unix://" + kUnixDomainPath + ";Account=" + kAccountName + ";Namespace=" + kNamespaceName;

  auto metric_data = GenerateSumDataMetrics();
  ExporterOptions options{conn_string};
  opentelemetry::exporter::geneva::metrics::Exporter exporter(options);
  exporter.Export(metric_data);
  yield_for(std::chrono::milliseconds(5000));

  testServer.Stop();

}
