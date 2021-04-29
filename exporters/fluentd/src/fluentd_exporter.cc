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
#include "opentelemetry/exporters/fluentd/fluentd_exporter.h"
#include "opentelemetry/exporters/fluentd/recordable.h"
#include "opentelemetry/ext/http/client/http_client_factory.h"
#include "opentelemetry/ext/http/common/url_parser.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace fluentd
{

// -------------------------------- Constructors --------------------------------

FluentdExporter::FluentdExporter(const FluentdExporterOptions &options)
    : options_(options) /* , url_parser_(options_.endpoint) */
{
  // http_client_ = ext::http::client::HttpClientFactory::CreateSync();
  InitializeLocalEndpoint();
}

FluentdExporter::FluentdExporter() : options_(FluentdExporterOptions()) /* , url_parser_(options_.endpoint) */
{
  // http_client_ = ext::http::client::HttpClientFactory::CreateSync();
  InitializeLocalEndpoint();
}

// ----------------------------- Exporter methods ------------------------------

std::unique_ptr<sdk::trace::Recordable> FluentdExporter::MakeRecordable() noexcept
{
  return std::unique_ptr<sdk::trace::Recordable>(new Recordable);
}

sdk::common::ExportResult FluentdExporter::Export(
    const nostd::span<std::unique_ptr<sdk::trace::Recordable>> &spans) noexcept
{
  if (isShutdown_)
  {
    return sdk::common::ExportResult::kFailure;
  }
  exporter::fluentd::FluentdSpan json_spans = {};
  for (auto &recordable : spans)
  {
    auto rec = std::unique_ptr<Recordable>(static_cast<Recordable *>(recordable.release()));
    if (rec != nullptr)
    {
      auto json_span = rec->span();
      // add localEndPoint
      json_span["localEndpoint"] = local_end_point_;
      json_spans.push_back(json_span);
    }
  }
  auto body_s = json_spans.dump();
  printf("%s\n", body_s.c_str());

  // TODO: send it here !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*
  http_client::Body body_v(body_s.begin(), body_s.end());
  auto result = http_client_->Post(url_parser_.url_, body_v);
  if (result && result.GetResponse().GetStatusCode() == 200 ||
      result.GetResponse().GetStatusCode() == 202)
  {
    return sdk::common::ExportResult::kSuccess;
  }
  else
  {
    if (result.GetSessionState() == http_client::SessionState::ConnectFailed)
    {
      // TODO -> Handle error / retries
    }
    return sdk::common::ExportResult::kFailure;
  }
 */

  return sdk::common::ExportResult::kSuccess;
}

void FluentdExporter::InitializeLocalEndpoint()
{
  if (options_.service_name.length())
  {
    local_end_point_["serviceName"] = options_.service_name;
  }
  if (options_.ipv4.length())
  {
    local_end_point_["ipv4"] = options_.ipv4;
  }
  if (options_.ipv6.length())
  {
    local_end_point_["ipv6"] = options_.ipv6;
  }

  local_end_point_["port"] = options_.port;
}

}  // namespace fluentd
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
