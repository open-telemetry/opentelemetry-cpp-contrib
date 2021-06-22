/*
 * Copyright The OpenTelemetry Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "opentelemetry/ext/net/common/socket_tools.h"

#include "opentelemetry/exporters/fluentd/recordable.h"
#include "opentelemetry/ext/http/common/url_parser.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/span_data.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace fluentd
{

/**
 * Struct to hold fluentd  exporter options.
 */
struct FluentdExporterOptions
{
  // The endpoint to export to. By default the OpenTelemetry Collector's default endpoint.
  TransportFormat format = TransportFormat::kForward;
  std::string tag = "tag.service";
  std::string endpoint;
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
      std::chrono::microseconds timeout = std::chrono::microseconds::max()) noexcept override
  {
    return true;
  }

protected:

  void Initialize();

  SocketTools::Socket socket_;
  SocketTools::SocketParams socketparams_{AF_INET, SOCK_STREAM, 0};
  nostd::unique_ptr<SocketTools::SocketAddr> addr_;

  FluentdExporterOptions options_;

  bool isShutdown_ = false;
};

}  // namespace fluentd
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
