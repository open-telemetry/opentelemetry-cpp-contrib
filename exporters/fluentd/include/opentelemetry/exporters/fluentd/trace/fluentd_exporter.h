// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "opentelemetry/exporters/fluentd/common/socket_tools.h"

#include "opentelemetry/exporters/fluentd/trace/recordable.h"
#include "opentelemetry/ext/http/common/url_parser.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/span_data.h"

#include <queue>
#include <vector>
#include <cstdint>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace fluentd
{

/**
 * Export mode - async and sync
 */

enum class ExportMode {
  SYNC_MODE,
  ASYNC_MODE
};

/**
 * Struct to hold fluentd  exporter options.
 */
struct FluentdExporterOptions
{
  // The endpoint to export to. By default the OpenTelemetry Collector's default endpoint.
  TransportFormat format = TransportFormat::kForward;
  std::string tag = "tag.service";
  std::string endpoint;
  ExportMode export_mode = ExportMode::ASYNC_MODE;
  size_t retry_count = 2;        // number of retries before drop
  size_t max_queue_size = 16384;  // max events buffer size
  size_t wait_interval_ms = 0;    // default wait interval between batches
  bool convert_event_to_trace = false ; // convert events to trace
};

namespace trace_sdk = opentelemetry::sdk::trace;

/**
 * The fluentd exporter exports span data in JSON format as expected by fluentd
 */
class FluentdExporter final : public trace_sdk::SpanExporter
{
public:
  /**
   * Create a FluentdExporter using all default options.
   */
  FluentdExporter();

  /**
   * Create a FluentdExporter using the given options.
   */
  explicit FluentdExporter(const FluentdExporterOptions &options);

  /**
   * Create a span recordable.
   * @return a newly initialized Recordable object
   */
  std::unique_ptr<trace_sdk::Recordable> MakeRecordable() noexcept override;

  /**
   * Export a batch of span recordables in JSON format.
   * @param spans a span of unique pointers to span recordables
   */
  sdk::common::ExportResult Export(
      const nostd::span<std::unique_ptr<trace_sdk::Recordable>> &spans) noexcept override;

  /**
   * Shut down the exporter.
   * @param timeout an optional timeout, default to max.
   */
  bool Shutdown(
      std::chrono::microseconds timeout = std::chrono::microseconds::max()) noexcept override;

protected:
  
  // State management
  bool Initialize();
  FluentdExporterOptions options_;
  std::atomic<bool> isShutdown_{false};

  // Connectivity management. One end-point per exporter instance.
  bool Connect();
  bool Disconnect();
  bool connected_{false};
  // Socket connection is re-established for every batch of events
  SocketTools::Socket socket_;
  SocketTools::SocketParams socketparams_{AF_INET, SOCK_STREAM, 0};
  nostd::unique_ptr<SocketTools::SocketAddr> addr_;

  // Upload queue management
  bool Enqueue(std::vector<uint8_t> &packet);
  bool Send(std::vector<uint8_t> &packet);
  void UploadLoop();
  std::thread &GetUploaderThread();
  void JoinUploaderThread();

  bool has_more_{false};
  std::recursive_mutex packets_mutex_;
  std::mutex has_more_mutex_;
  std::condition_variable has_more_cv_;
  std::queue<std::vector<uint8_t>> packets_;

private:

  // Number of batches
  size_t seq_batch_{0};

  // Number of spans processed
  size_t seq_span_{0};

  // Number of events on span processed
  size_t seq_evt_{0};

  // Number of connections
  size_t seq_conn_{0};

};

}  // namespace fluentd
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
