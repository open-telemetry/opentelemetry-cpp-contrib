// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#pragma once

#define ENABLE_LOGS_PREVIEW 1

#include "opentelemetry/exporters/fluentd/common/socket_tools.h"

#include "opentelemetry/exporters/fluentd/trace/recordable.h"
#include "opentelemetry/ext/http/common/url_parser.h"
#include "opentelemetry/sdk/logs/exporter.h"
#include "opentelemetry/sdk/logs/log_record.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace fluentd {
namespace logs {

/**
 * Export mode - async and sync
 */

enum class ExportMode { SYNC_MODE, ASYNC_MODE };

// Ref. https://github.com/fluent/fluentd/wiki/Forward-Protocol-Specification-v1
enum class TransportFormat {
  kMessage,
  kForward,
  kPackedForward,
  kCompressedPackedForward
};

/**
 * Struct to hold fluentd  exporter options.
 */
struct FluentdExporterOptions {
  // The endpoint to export to. By default the OpenTelemetry Collector's default
  // endpoint.
  TransportFormat format = TransportFormat::kForward;
  std::string tag = "tag.service";
  std::string endpoint;
  ExportMode export_mode = ExportMode::ASYNC_MODE;
  size_t retry_count = 2;              // number of retries before drop
  size_t max_queue_size = 16384;       // max events buffer size
  size_t wait_interval_ms = 0;         // default wait interval between batches
  bool convert_event_to_trace = false; // convert events to trace
};

namespace nostd = opentelemetry::nostd;
namespace logs_sdk = opentelemetry::sdk::logs;

/**
 * The fluentd exporter exports span data in JSON format as expected by fluentd
 */
class FluentdExporter final : public logs_sdk::LogExporter {
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
  std::unique_ptr<logs_sdk::Recordable> MakeRecordable() noexcept override;

  /**
   * Export a batch of span recordables in JSON format.
   * @param spans a span of unique pointers to span recordables
   */
  sdk::common::ExportResult
  Export(const nostd::span<std::unique_ptr<logs_sdk::Recordable>>
             &logs) noexcept override;

  /**
   * Shut down the exporter.
   * @param timeout an optional timeout, default to max.
   */
  bool Shutdown(std::chrono::microseconds timeout =
                    std::chrono::microseconds::max()) noexcept override;

protected:
  // State management
  bool Initialize();
  bool Send(std::vector<uint8_t> &packet);
  FluentdExporterOptions options_;
  bool is_shutdown_{false};

  // Connectivity management. One end-point per exporter instance.
  bool Connect();
  bool Disconnect();
  bool connected_{false};
  // Socket connection is re-established for every batch of events
  SocketTools::Socket socket_;
  SocketTools::SocketParams socketparams_{AF_INET, SOCK_STREAM, 0};
  nostd::unique_ptr<SocketTools::SocketAddr> addr_;
};

} // namespace logs
} // namespace fluentd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE