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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>
#include <Windows.h>
#endif

#include "opentelemetry/sdk/trace/recordable.h"
#include "opentelemetry/sdk/trace/simple_processor.h"
#include "opentelemetry/sdk/trace/span_data.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/provider.h"

#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/random_id_generator.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"

#include "opentelemetry/common/timestamp.h"
#include "opentelemetry/exporters/fluentd/trace/fluentd_exporter.h"
#include "opentelemetry/exporters/fluentd/trace/recordable.h"

#include <iostream>

#include <map>
#include <string>

#include <gtest/gtest.h>

#include "../common/socket_server.h"

using namespace SOCKET_SERVER_NS;

using namespace opentelemetry::sdk::trace;
using namespace opentelemetry::sdk::resource;

namespace trace = opentelemetry::trace;
namespace nostd = opentelemetry::nostd;
namespace sdktrace = opentelemetry::sdk::trace;
using json = nlohmann::json;

#include <chrono>
#include <iostream>
#include <thread>

// "busy sleep" while suggesting that other threads run
// for a small amount of time
template <typename timeunit> void yield_for(timeunit duration) {
  auto start = std::chrono::high_resolution_clock::now();
  auto end = start + duration;
  do {
    std::this_thread::yield();
  } while (std::chrono::high_resolution_clock::now() < end);
}

#if 0
// Testing Shutdown functionality of OStreamSpanExporter, should expect no data to be sent to Stream
TEST(FluentdSpanRecordable, SetIdentity)
{
  json j_span = {{"options",{{"id", "0000000000000002"},
                 {"parentId", "0000000000000003"},
                 {"traceId", "00000000000000000000000000000001"}}},
                 {"tags", "Span"}};
  opentelemetry::exporter::fluentd::trace::Recordable rec;
  const trace::TraceId trace_id(std::array<const uint8_t, trace::TraceId::kSize>(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}));

  const trace::SpanId span_id(
      std::array<const uint8_t, trace::SpanId::kSize>({0, 0, 0, 0, 0, 0, 0, 2}));

  const trace::SpanId parent_span_id(
      std::array<const uint8_t, trace::SpanId::kSize>({0, 0, 0, 0, 0, 0, 0, 3}));

  const opentelemetry::trace::SpanContext span_context{
      trace_id, span_id,
      opentelemetry::trace::TraceFlags{opentelemetry::trace::TraceFlags::kIsSampled}, true};

  rec.SetIdentity(span_context, parent_span_id);
  EXPECT_EQ(rec.span(), j_span);
}

TEST(FluentdSpanRecordable, SetName)
{
  nostd::string_view name = "Test Span";
  json j_span             = {{"name", name}};
  opentelemetry::exporter::fluentd::trace::Recordable rec;
  rec.SetName(name);
  EXPECT_EQ(rec.span(), j_span);
}

TEST(FluentdSpanRecordable, SetStartTime)
{
  opentelemetry::exporter::fluentd::trace::Recordable rec;
  std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now();
  opentelemetry::common::SystemTimestamp start_timestamp(start_time);

  uint64_t unix_start =
      std::chrono::duration_cast<std::chrono::microseconds>(start_time.time_since_epoch()).count();
  json j_span = {{"timestamp", unix_start}};
  rec.SetStartTime(start_timestamp);
  EXPECT_EQ(rec.span(), j_span);
}

TEST(FluentdSpanRecordable, SetDuration)
{
  json j_span = {{"duration", 10}, {"timestamp", 0}};
  opentelemetry::exporter::fluentd::trace::Recordable rec;
  // Start time is 0
  opentelemetry::common::SystemTimestamp start_timestamp;

  std::chrono::nanoseconds duration(10);
  uint64_t unix_end = duration.count();

  rec.SetStartTime(start_timestamp);
  rec.SetDuration(duration);
  EXPECT_EQ(rec.span(), j_span);
}

TEST(FluentdSpanRecordable, SetInstrumentationScope)
{
  using InstrumentationScope = opentelemetry::sdk::instrumentationscope::InstrumentationScope;

  const char *library_name    = "otel-cpp";
  const char *library_version = "0.5.0";
  json j_span                 = {
      {"tags", {{"otel.library.name", library_name}, {"otel.library.version", library_version}}}};
  opentelemetry::exporter::fluentd::trace::Recordable rec;

  rec.SetInstrumentationScope(*InstrumentationScope::Create(library_name, library_version));

  EXPECT_EQ(rec.span(), j_span);
}

TEST(FluentdSpanRecordable, SetStatus)
{
  std::string description                     = "Error description";
  std::vector<trace::StatusCode> status_codes = {trace::StatusCode::kError, trace::StatusCode::kOk};
  for (auto &status_code : status_codes)
  {
    opentelemetry::exporter::fluentd::trace::Recordable rec;
    trace::StatusCode code(status_code);
    json j_span;
    if (status_code == trace::StatusCode::kError)
      j_span = {{"tags", {{"otel.status_code", status_code}, {"error", description}}}};
    else
      j_span = {{"tags", {{"otel.status_code", status_code}}}};

    rec.SetStatus(code, description);
    EXPECT_EQ(rec.span(), j_span);
  }
}

TEST(FluentdSpanRecordable, SetSpanKind)
{
  json j_json_client = {{"kind", "CLIENT"}};
  opentelemetry::exporter::fluentd::trace::Recordable rec;
  rec.SetSpanKind(opentelemetry::trace::SpanKind::kClient);
  EXPECT_EQ(rec.span(), j_json_client);
}

TEST(FluentdSpanRecordable, AddEventDefault)
{
  opentelemetry::exporter::fluentd::trace::Recordable rec;
  nostd::string_view name = "Test Event";

  std::chrono::system_clock::time_point event_time = std::chrono::system_clock::now();
  opentelemetry::common::SystemTimestamp event_timestamp(event_time);

  rec.opentelemetry::sdk::trace::Recordable::AddEvent(name, event_timestamp);

  uint64_t unix_event_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(event_time.time_since_epoch()).count();

  json j_span = {
      {"annotations",
       {{{"value", json({{name, json::object()}}).dump()}, {"timestamp", unix_event_time}}}}};
  EXPECT_EQ(rec.span(), j_span);
}

TEST(FluentdSpanRecordable, AddEventWithAttributes)
{
  opentelemetry::exporter::fluentd::trace::Recordable rec;
  nostd::string_view name = "Test Event";

  std::chrono::system_clock::time_point event_time = std::chrono::system_clock::now();
  opentelemetry::common::SystemTimestamp event_timestamp(event_time);
  uint64_t unix_event_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(event_time.time_since_epoch()).count();

  const int kNumAttributes              = 3;
  std::string keys[kNumAttributes]      = {"attr1", "attr2", "attr3"};
  int values[kNumAttributes]            = {4, 7, 23};
  std::map<std::string, int> attributes = {
      {keys[0], values[0]}, {keys[1], values[1]}, {keys[2], values[2]}};

  rec.AddEvent("Test Event", event_timestamp,
               opentelemetry::common::KeyValueIterableView<std::map<std::string, int>>(attributes));

  nlohmann::json j_span = {
      {"annotations",
       {{{"value", json({{"Test Event", {{"attr1", 4}, {"attr2", 7}, {"attr3", 23}}}}).dump()},
         {"timestamp", unix_event_time}}}}};
  EXPECT_EQ(rec.span(), j_span);
}

// Test non-int single types. Int single types are tested using templates (see IntAttributeTest)
TEST(FluentdSpanRecordable, SetSingleAtrribute)
{
  opentelemetry::exporter::fluentd::trace::Recordable rec;
  nostd::string_view bool_key = "bool_attr";
  opentelemetry::common::AttributeValue bool_val(true);
  rec.SetAttribute(bool_key, bool_val);

  nostd::string_view double_key = "double_attr";
  opentelemetry::common::AttributeValue double_val(3.3);
  rec.SetAttribute(double_key, double_val);

  nostd::string_view str_key = "str_attr";
  opentelemetry::common::AttributeValue str_val(nostd::string_view("Test"));
  rec.SetAttribute(str_key, str_val);
  nlohmann::json j_span = {
      {"tags", {{"bool_attr", true}, {"double_attr", 3.3}, {"str_attr", "Test"}}}};

  EXPECT_EQ(rec.span(), j_span);
}

// Test non-int array types. Int array types are tested using templates (see IntAttributeTest)
TEST(FluentdSpanRecordable, SetArrayAtrribute)
{
  opentelemetry::exporter::fluentd::trace::Recordable rec;
  nlohmann::json j_span = {{"tags",
                            {{"bool_arr_attr", {true, false, true}},
                             {"double_arr_attr", {22.3, 33.4, 44.5}},
                             {"str_arr_attr", {"Hello", "World", "Test"}}}}};
  const int kArraySize  = 3;

  bool bool_arr[kArraySize] = {true, false, true};
  nostd::span<const bool> bool_span(bool_arr);
  rec.SetAttribute("bool_arr_attr", bool_span);

  double double_arr[kArraySize] = {22.3, 33.4, 44.5};
  nostd::span<const double> double_span(double_arr);
  rec.SetAttribute("double_arr_attr", double_span);

  nostd::string_view str_arr[kArraySize] = {"Hello", "World", "Test"};
  nostd::span<const nostd::string_view> str_span(str_arr);
  rec.SetAttribute("str_arr_attr", str_span);

  EXPECT_EQ(rec.span(), j_span);
}

TEST(FluentdSpanRecordable, SetResource)
{
  opentelemetry::exporter::fluentd::trace::Recordable rec;
  std::string service_name = "test";
  auto resource = opentelemetry::sdk::resource::Resource::Create({{"service.name", service_name}});
  rec.SetResource(resource);
  EXPECT_EQ(rec.GetServiceName(), service_name);
}

/**
 * AttributeValue can contain different int types, such as int, int64_t,
 * unsigned int, and uint64_t. To avoid writing test cases for each, we can
 * use a template approach to test all int types.
 */
template <typename T>
struct FluentdIntAttributeTest : public testing::Test
{
  using IntParamType = T;
};

using IntTypes = testing::Types<int, int64_t, unsigned int, uint64_t>;
TYPED_TEST_SUITE(FluentdIntAttributeTest, IntTypes);

TYPED_TEST(FluentdIntAttributeTest, SetIntSingleAttribute)
{
  using IntType = typename TestFixture::IntParamType;
  IntType i     = 2;
  opentelemetry::common::AttributeValue int_val(i);

  opentelemetry::exporter::fluentd::trace::Recordable rec;
  rec.SetAttribute("int_attr", int_val);
  nlohmann::json j_span = {{"tags", {{"int_attr", 2}}}};
  EXPECT_EQ(rec.span(), j_span);
}

TYPED_TEST(FluentdIntAttributeTest, SetIntArrayAttribute)
{
  using IntType = typename TestFixture::IntParamType;

  const int kArraySize        = 3;
  IntType int_arr[kArraySize] = {4, 5, 6};
  nostd::span<const IntType> int_span(int_arr);

  opentelemetry::exporter::fluentd::trace::Recordable rec;
  rec.SetAttribute("int_arr_attr", int_span);
  nlohmann::json j_span = {{"tags", {{"int_arr_attr", {4, 5, 6}}}}};
  EXPECT_EQ(rec.span(), j_span);
}

#endif
using Properties = std::map<std::string, opentelemetry::common::AttributeValue>;

struct TestServer {
  SocketServer &server;
  std::atomic<uint32_t> count{0};

  TestServer(SocketServer &server) : server(server) {
    server.onRequest = [&](SocketServer::Connection &conn) {
      std::vector<uint8_t> msg(conn.request_buffer.data(),
                               conn.request_buffer.data() +
                                   conn.request_buffer.size());

      try {
        auto j = nlohmann::json::from_msgpack(msg);
        std::cout << "[" << count.fetch_add(1)
                  << "] SocketServer received payload: " << std::endl
                  << j.dump(2) << std::endl;

        conn.response_buffer = j.dump(2);
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

TEST(FluentdExporter, SendTraceEvents) {
  bool isRunning = true;

  // Start test server
  SocketAddr destination("127.0.0.1:24222");
  SocketParams params{AF_INET, SOCK_STREAM, 0};
  SocketServer socketServer(destination, params);
  TestServer testServer(socketServer);
  testServer.Start();

  yield_for(std::chrono::milliseconds(500));

  // Connect to local test server
  opentelemetry::exporter::fluentd::common::FluentdExporterOptions options;
  options.endpoint = "tcp://127.0.0.1:24222";
  options.tag = "tag.my_service";
  options.convert_event_to_trace = true;

  auto exporter = std::unique_ptr<opentelemetry::sdk::trace::SpanExporter>(
      new opentelemetry::exporter::fluentd::trace::FluentdExporter(options));

  auto processor = std::unique_ptr<SpanProcessor>(
      new sdktrace::SimpleSpanProcessor(std::move(exporter)));

  auto provider = nostd::shared_ptr<trace::TracerProvider>(
      new TracerProvider(std::move(processor)));

  // Set the global trace provider
  opentelemetry::trace::Provider::SetTracerProvider(provider);

  std::string providerName = "MyInstrumentationName";
  auto tracer = provider->GetTracer(providerName);

  // Span attributes
  Properties attribs = {{"attrib1", 1}, {"attrib2", 2}};

  auto span1 = tracer->StartSpan("MySpanL1");
  {
    // auto scope = tracer->WithActiveSpan(span1);
    auto span2 = tracer->StartSpan("MySpanL2", attribs);
    {
      auto span3 = tracer->StartSpan("MySpanL3", attribs);

      // Add first event
      std::string eventName1 = "MyEvent1";
      Properties event1 = {{"uint32Key", (uint32_t)1234},
                           {"uint64Key", (uint64_t)1234567890},
                           {"strKey", "someValue"}};
      span2->AddEvent(eventName1, event1);

      // Add second event
      std::string eventName2 = "MyEvent2";
      Properties event2 = {{"uint32Key", (uint32_t)9876},
                           {"uint64Key", (uint64_t)987654321},
                           {"strKey", "anotherValue"}};
      span2->AddEvent(eventName2, event2);

      // Add third event
      std::string eventName3 = "MyEvent3";
      Properties event3 = {{"metadata", "ai_event"},
                           {"uint32Key", (uint32_t)9876},
                           {"uint64Key", (uint64_t)987654321}};
      span3->AddEvent(eventName3, event3);

      span3->End(); // end MySpanL3
    }
    span2->End(); // end MySpanL2
  }
  span1->End(); // end MySpanL1

  tracer->ForceFlushWithMicroseconds(1000);
  tracer->CloseWithMicroseconds(0);

  testServer.WaitForEvents(6, 200); // 6 batches must arrive in 200ms
  testServer.Stop();
}
