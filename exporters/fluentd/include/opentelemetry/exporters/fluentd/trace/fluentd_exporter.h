// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "opentelemetry/exporters/fluentd/common/fluentd_common.h"
#include "opentelemetry/exporters/fluentd/common/socket_tools.h"
#include "opentelemetry/exporters/fluentd/trace/recordable.h"
#include "opentelemetry/ext/http/common/url_parser.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/span_data.h"

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
namespace trace {

namespace trace_sdk = opentelemetry::sdk::trace;
namespace fluentd_common = opentelemetry::exporter::fluentd::common;

/**
 * The fluentd exporter exports span data in JSON format as expected by fluentd
 */
class FluentdExporter final : public trace_sdk::SpanExporter {
public:
  /**
   * Create a FluentdExporter using all default options.
   */
  FluentdExporter();

  /**
   * Create a FluentdExporter using the given options.
   */
  explicit FluentdExporter(
      const fluentd_common::FluentdExporterOptions &options);

  /**
   * Create a span recordable.
   * @return a newly initialized Recordable object
   */
  std::unique_ptr<trace_sdk::Recordable> MakeRecordable() noexcept override;

  /**
   * Export a batch of span recordables in JSON format.
   * @param spans a span of unique pointers to span recordables
   */
  sdk::common::ExportResult
  Export(const nostd::span<std::unique_ptr<trace_sdk::Recordable>>
             &spans) noexcept override;

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
  fluentd_common::FluentdExporterOptions options_;
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

} // namespace trace
} // namespace fluentd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
