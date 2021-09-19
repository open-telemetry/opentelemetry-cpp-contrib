// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#define HAVE_CONSOLE_LOG

#include "opentelemetry/exporters/fluentd/trace/fluentd_exporter.h"
#include "opentelemetry/exporters/fluentd/trace/recordable.h"
#include "opentelemetry/ext/http/common/url_parser.h"

#include "opentelemetry/exporters/fluentd/common/fluentd_logging.h"

#include "nlohmann/json.hpp"

#include <cassert>

#include <iostream>

using UrlParser = opentelemetry::ext::http::common::UrlParser;

using namespace nlohmann;

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace fluentd
{

/**
 * @brief Scheme for tcp:// stream
*/
constexpr const char* kTCP = "tcp";

/**
 * @brief Scheme for udp:// datagram
*/
constexpr const char* kUDP = "udp";

/**
 * @brief Scheme for unix:// domain socket
*/
constexpr const char* kUNIX = "unix";

/**
 * @brief Idle-wait yielding to other threads.
 * @tparam timeunit 
 * @param duration 
*/
template <typename timeunit> static inline void yield_for(timeunit duration)
{
  auto start = std::chrono::high_resolution_clock::now();
  auto end   = start + duration;
  do
  {
    std::this_thread::yield();
  } while (std::chrono::high_resolution_clock::now() < end);
}

/**
 * @brief Create FluentD exporter with options
 * @param options 
*/
FluentdExporter::FluentdExporter(const FluentdExporterOptions &options) : options_(options)
{
  Initialize();
}

/**
 * @brief Create FluentD exporter with default options
*/
FluentdExporter::FluentdExporter() : options_(FluentdExporterOptions())
{
  Initialize();
}

/**
 * @brief Create new Recordable
 * @return Recordable
*/
std::unique_ptr<sdk::trace::Recordable> FluentdExporter::MakeRecordable() noexcept
{
  LOG_TRACE("LALIT: Make recordable");
  return std::unique_ptr<sdk::trace::Recordable>(new Recordable);
}

/**
 * @brief Export spans.
 * @param spans 
 * @return Export result.
*/
sdk::common::ExportResult FluentdExporter::Export(
    const nostd::span<std::unique_ptr<sdk::trace::Recordable>> &spans) noexcept
{
  LOG_ERROR("\nLALIT:Exporting...");
  // Calculate number of batches
  seq_batch_++;

  // If no spans in Recordable, then return error.
  if (spans.size() == 0)
  {
    return sdk::common::ExportResult::kFailure;
  }

  json events = {};
  {
    // Write all Spans first
    json obj = json::array();
    obj.push_back(FLUENT_VALUE_SPAN);
    json spanevents = json::array();
    for (auto &recordable : spans)
    {
      auto rec = std::unique_ptr<Recordable>(static_cast<Recordable *>(recordable.release()));
      if (rec != nullptr)
      {
        auto span = rec->span();
        // Emit "Span" as fluentd event
        json record = json::array();
        record.push_back(span["options"][FLUENT_FIELD_ENDTTIME]);
        json fields = {};
        for (auto &kv : span["options"].items())
        {
          fields[kv.key()] = kv.value();
        }
        record.push_back(fields);
        spanevents.push_back(record);
        // Gather Span event statistics
        seq_span_++;
        LOG_TRACE("exporting: batch #%d span #%d", seq_batch_, seq_span_);
        // Iterate over all events added on this span
        for (auto &v : span["events"])
        {
          auto &event      = v[1];
          std::string name = event[FLUENT_FIELD_NAME];

          event[FLUENT_FIELD_SPAN_ID]  = span["options"][FLUENT_FIELD_SPAN_ID];
          event[FLUENT_FIELD_TRACE_ID] = span["options"][FLUENT_FIELD_TRACE_ID];

          // Event Time flows as a separate field in fluentd.
          // However, if we may need to consider addding
          // span["options"][FLUENT_FIELD_TIME]
          // 
          // Complete list of all Span attributes :
          // for (auto &kv : span["options"].items()) { ... }

          // Group all events by matching event name in array.
          // This array is translated into FluentD forward payload.
          if (!events.contains(name))
          {
            events[name] = json::array();
          }
          events[name].push_back(v);
          // Gather event-on-Span statistics
          seq_evt_++;
          LOG_TRACE("exporting: batch #%d evt #%d", seq_batch_, seq_evt_);
        }
      }
    }
    obj.push_back(spanevents);
    LOG_TRACE("sending %zu Span event(s)", obj[1].size());
    std::cout << "LALIT: SPAN EVENTS TO BE SENT: " << obj.dump();
    std::vector<uint8_t> msg = nlohmann::json::to_msgpack(obj);
    if (options_.export_mode == ExportMode::ASYNC_MODE)
    {
      // Schedule upload of Span event(s)
      Enqueue(msg);
    } else {
      // Immediately send the Span event(s)
      bool result = Send(msg);
      if (!result) {
        return sdk::common::ExportResult::kFailure;
      }
    }
  }
  if (options_.convert_event_to_trace)
  {
    for (auto &kv : events.items())
    {
      json obj = json::array();
      obj.push_back(kv.key());
      json otherevents = json::array();
      for (auto &v : kv.value())
      {
        otherevents.push_back(v);
      }
      obj.push_back(otherevents);
      LOG_TRACE("sending %zu %s events", obj[1].size(), kv.key().c_str());
      std::cout << "LALIT: EVENTS TO BE SENT: " << obj.dump();

      std::vector<uint8_t> msg = nlohmann::json::to_msgpack(obj);
      if (options_.export_mode == ExportMode::ASYNC_MODE)
      {
        // Schedule upload of Span event(s)
        Enqueue(msg);
      } 
      else 
      {
        // Immediately send the Span event(s)
        bool result = Send(msg);
        if (!result) {
          return sdk::common::ExportResult::kFailure;
        }
      }
    } 

  }

  // At this point we always return success because there is no way
  // to know if delivery is gonna succeed with multiple retries.
  return sdk::common::ExportResult::kSuccess;
}

/**
 * @brief Initialize FluentD exporter socket.
 * @return true if end-point settings have been accepted.
*/
bool FluentdExporter::Initialize()
{
  UrlParser url(options_.endpoint);
  bool is_unix_domain = false;

  if (url.scheme_ == kTCP)
  {
    socketparams_ = {AF_INET, SOCK_STREAM, 0};
  }
  else if (url.scheme_ == kUDP)
  {
    socketparams_ = {AF_INET, SOCK_DGRAM, 0};
  }
#ifdef HAVE_UNIX_DOMAIN
  else if (url.scheme_ == kUNIX)
  {
    socketparams_ = {AF_UNIX, SOCK_STREAM, 0};
    is_unix_domain  = true;
  }
#endif
  else
  {
#if defined(__EXCEPTIONS)
    // Customers MUST specify valid end-point configuration
    throw new std::runtime_error("Invalid endpoint!");
#endif
    return false;
  }

  std::cout << "\nLevel1" << std::flush;
  addr_.reset(new SocketTools::SocketAddr(options_.endpoint.c_str(), is_unix_domain));
  LOG_TRACE("connecting to %s", addr_->toString().c_str());
  std::cout << "\nLevel2\n" << std::flush;

  if (options_.export_mode == ExportMode::ASYNC_MODE)
  {
    // Start async uploader thread
    const auto &uploader = GetUploaderThread();
    std::stringstream ss;
    ss << std::this_thread::get_id();
    LOG_TRACE("upload thread started with id=%ll", std::stoull(ss.str()));
  }

  return true;
}

/**
 * @brief Establish connection to FluentD
 * @return true if connected successfully.
*/
bool FluentdExporter::Connect()
{
  if (!connected_)
  {
    socket_ = SocketTools::Socket(socketparams_);
    connected_ = socket_.connect(*addr_);
    if (!connected_)
    {
      LOG_ERROR("Unable to connect to %s", options_.endpoint.c_str());
      return false;
    }
    seq_conn_++;
  }
  // Connected or already connected
  return true;
}

/**
 * @brief Enqueue fluentd forward protocol packet for delivery.
 * @param packet 
 * @return true if packet accepted for delivery
*/
bool FluentdExporter::Enqueue(std::vector<uint8_t> &packet)
{
  LOCKGUARD(packets_mutex_);
  if (packets_.size() < options_.max_queue_size)
  {
    packets_.push(std::move(packet));
    {
      std::lock_guard<std::mutex> lk(has_more_mutex_);
      has_more_ = (packets_.size() > 0);
      has_more_cv_.notify_all();
    }
    return true;
  }
  // queue overflow!
  return false;
}

/**
 * @brief Try to upload fluentd forward protocol packet.
 * This method respects the retry options for connects
 * and upload retries.
 * 
 * @param packet
 * @return true if packet got delivered.
*/
bool FluentdExporter::Send(std::vector<uint8_t> &packet)
{
  size_t retryCount = options_.retry_count;
  while (retryCount--)
  {
    int error_code = 0;
    // Check if socket is Okay
    if (connected_)
    {
      socket_.getsockopt(SOL_SOCKET, SO_ERROR, error_code);
      if (error_code!=0)
      {
        connected_ = false;
      }
    }
    // Reconnect if not Okay
    if (!connected_)
    {
      // Establishing socket connection may take time
      if (!Connect())
      {
        continue;
      }
      LOG_DEBUG("socket connected");
    }

    // Try to write
    size_t sentSize = socket_.writeall(packet);
    if (packet.size() == sentSize)
    {
      LOG_DEBUG("send successful");
      Disconnect();
      LOG_DEBUG("socket disconnected");
      return true;
    }

    LOG_WARN("send failed, retrying %u ...", retryCount);
    // Retry to connect and/or send
  }

  LOG_ERROR("send failed!");
  return false;
}

void FluentdExporter::UploadLoop()
{
  while (!isShutdown_.load())
  {
    // Wait on condition variable until got something to send
    {
      std::unique_lock<std::mutex> lk(has_more_mutex_);
      has_more_cv_.wait(lk, [&] { return (has_more_) || (isShutdown_.load()); });
    }
    // Send all items
    while (!packets_.empty())
    {
      std::vector<uint8_t> packet;
      {
        LOCKGUARD(packets_mutex_);
        packets_.front().swap(packet);
        packets_.pop();
      }
      // More packets may get added to queue while we send.
      Send(packet);
    }
    // All items have been sent.
    {
      std::unique_lock<std::mutex> lk(has_more_mutex_);
      LOCKGUARD(packets_mutex_);
      has_more_ = (packets_.size() > 0);
    }
    // Optional: graceful pause between batches.
    // This allows to throttle / shape traffic
    // and CPU utilization, avoiding excessive
    // usage by telemetry SDK.
    if (options_.wait_interval_ms)
    {
      yield_for(std::chrono::milliseconds(options_.wait_interval_ms));
    }
  }
  // Allow to trigger debug error in case if upload
  // failed on shutdown.
  assert(packets_.empty());
}

/**
 * @brief Obtain a reference to uploader singleton.
 * @return Uploader thread.
*/
std::thread& FluentdExporter::GetUploaderThread()
{
  // Thread-safe initialization of singleton uploader.
  // Only one thread is performing data upload.
  static std::thread uploader([&] { UploadLoop(); });
  return uploader;
}

/**
 * @brief Attempt to join uploader thread.
*/
void FluentdExporter::JoinUploaderThread()
{
  // Try to join uploader thread
  auto &uploader = GetUploaderThread();
  if (uploader.joinable())
  {
    try
    {
      uploader.join();
    }
    catch (...)
    {
      // thread has been already joined!
    }
  }
}

/**
 * @brief Disconnect FluentD socket or datagram.
 * @return 
*/
bool FluentdExporter::Disconnect()
{
  if (connected_)
  {
    connected_ = false;
    if (!socket_.invalid()) {
      socket_.close();
      return true;
    }
  }
  return false;
}

/**
 * @brief Shutdown FluentD exporter
 * @param  
 * @return 
*/
bool FluentdExporter::Shutdown(
    std::chrono::microseconds) noexcept
{

  if (!isShutdown_.exchange(true))
  {
    {
      std::lock_guard<std::mutex> lk(has_more_mutex_);
      has_more_cv_.notify_all();
    }
    if (options_.export_mode == ExportMode::ASYNC_MODE)
    {
      // Wait for upload to complete
      JoinUploaderThread();
      // Print debug statistics
      LOG_DEBUG("fluentd exporter stats:");
      LOG_DEBUG("batches = %zu", seq_batch_);
      LOG_DEBUG("spans   = %zu", seq_span_);
      LOG_DEBUG("events  = %zu", seq_evt_);
      LOG_DEBUG("uploads = %zu", seq_conn_);
    }
    return true;
  }
  return false;
}

}  // namespace fluentd
}  // namespace exporter
OPENTELEMETRY_END_NAMESPACE
