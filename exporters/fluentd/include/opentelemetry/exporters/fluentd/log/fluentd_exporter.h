// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "opentelemetry/exporters/fluentd/common/socket_tools.h"

#include "opentelemetry/exporters/fluentd/trace/recordable.h"
#include "opentelemetry/ext/http/common/url_parser.h"
#include "opentelemetry/sdk/logs/exporter.h"
#include "opentelemetry/logs/log_record.h"

#include <vector>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace fluentd {
namespace logs {

namespace nostd = opentelemetry::nostd;
namespace logs_sdk = opentelemetry::sdk::logs;
namespace fluentd_common = opentelemetry::exporter::fluentd::common;

/**
 * The fluentd exporter exports span data in JSON format as expected by fluentd
 */
class FluentdExporter final : public logs_sdk::LogRecordExporter {
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

  // Connectivity management. One end-point per exporter instance.
  bool Connect();
  bool Disconnect();

  // Socket connection is re-established for every batch of events
  SocketTools::Socket socket_;
  SocketTools::SocketParams socketparams_{AF_INET, SOCK_STREAM, 0};

  fluentd_common::FluentdExporterOptions options_;
  bool is_shutdown_{false};
  nostd::unique_ptr<SocketTools::SocketAddr> addr_;
  bool connected_{false};
};

} // namespace logs
} // namespace fluentd
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE
