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

#include <iostream>

#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../common/msgpack_timestamp.h"
#include "../common/socket_server.h"
#include "nlohmann/json.hpp"

using namespace SOCKET_SERVER_NS;

using namespace nlohmann;

inline json create_message(int64_t ts, json body) {
  auto arr = json::array();
  arr.push_back(ts);
  arr.push_back(body);
  return arr;
}

TEST(FluentdBaseline, MsgPackCodec) {
  json obj = json::array();
  obj.push_back("tag.name");
  json messages = json::array();
  messages.push_back(create_message(1441588984, {{"message", "foo1"}}));
  messages.push_back(create_message(1441588985, {{"message", "foo2"}}));
  messages.push_back(create_message(1441588986, {{"message", "foo3"}}));
  obj.push_back(messages);
  // Encode to MsgPack
  std::vector<uint8_t> msg = nlohmann::json::to_msgpack(obj);
  // Decode from MsgPack
  json obj2 = nlohmann::json::from_msgpack(msg);
  // Verify that the object is decoded as expected
  EXPECT_EQ("[\"tag.name\",[[1441588984,{\"message\":\"foo1\"}],[1441588985,{"
            "\"message\":\"foo2\"}],["
            "1441588986,{\"message\":\"foo3\"}]]]",
            obj2.dump());
}

#if 0
TEST(FluentdBaseline, FluentForwardTcp)
{
  json obj = json::array();
  obj.push_back("tag.name");
  json messages = json::array();
  messages.push_back(create_message(1441588984, {{"message", "foo1"}}));
  messages.push_back(create_message(1441588985, {{"message", "foo2"}}));
  messages.push_back(create_message(1441588986, {{"message", "foo3"}}));
  obj.push_back(messages);
  // Encode to MsgPack
  std::vector<uint8_t> msg = nlohmann::json::to_msgpack(obj);

  SocketParams params{AF_INET, SOCK_STREAM, 0};
  // See opentelemetry-cpp-contrib/examples/fluentd/run.[cmd|sh]
  SocketAddr destination("127.0.0.1:24222");
  // SocketServer server(destination, params);
  Socket client(params);
  client.connect(destination);
  client.writeall(msg);
  client.close();
}

#define JSON_TEXT(...) (#__VA_ARGS__)

TEST(FluentBaseline, FluentForwardTcpTimeExt)
{
  // Replace with binary subtype and bytes
  byte_container_with_subtype<std::vector<std::uint8_t>> ts
  {
      std::vector<uint8_t>{
          96,
          153,
          213,
          207,
          111,
          111,
          111,
          111,
      }
  };
  ts.set_subtype(0x00);

  json obj = json::array();
  obj.push_back("Span");
  json records = json::array();
  json record = json::array();
  record.push_back(ts);
  json fields =
  {
    {"azureResourceProvider", "Microsoft.AAD"},
    {"clientRequestId", "58a37988-2c05-427a-891f-5e0e1266fcc5"},
    {"env_cloud_role", "BusyWorker"},
    {"env_cloud_roleInstance", "CY1SCH030021417"},
    {"env_cloud_roleVer", "9.0.15289.2"},
    {"env_dt_spanId", "99b2ce684f069b4a"},
    {"env_dt_traceId", "790b7a6bc878cc41bbd5c6175a6e9f3e"},
    {"env_name", "Span"},
    {"env_properties", "{\"bar\" : 2, \"foo\" : 1}"},
    {"env_time", ts},
    {"env_ver", 4.0},
    {"httpStatusCode", 200},
    {"kind", 0},
    {"name", "LinuxCpp"},
    {"startTime", ts},
    {"success", true}
  };
  record.push_back(fields);
  // 3 identical records
  records.push_back(record);
  obj.push_back(records);
  // {"TimeFormat" : "DateTime"}

#if 0
  // Replace with binary subtype and bytes
  byte_container_with_subtype<std::vector<std::uint8_t>> ts
  {
      std::vector<uint8_t>{111, 111, 111, 111, 0, 0, 0, 0, 96, 153, 213, 207}
  };
  ts.set_subtype(0xff);
#endif
  std::cout << obj.dump(2) << std::endl;

  // Encode to MsgPack
  std::vector<uint8_t> msg = json::to_msgpack(obj);

  // Send to localhost
  SocketParams params{AF_INET, SOCK_STREAM, 0};
  // See opentelemetry-cpp-contrib/examples/fluentd/run.[cmd|sh]
  SocketAddr destination("127.0.0.1:24222");
  // SocketServer server(destination, params);
  // Send the same event a few times
  Socket client(params);
  client.connect(destination);
  client.writeall(msg);
  client.close();

  json j = json::from_msgpack(msg);
  std::cout << j.dump(2) << std::endl;
}

TEST(FluentdBaseline, FluentForwardTcpServer)
{
  bool isRunning = true;
  SocketParams params{AF_INET, SOCK_STREAM, 0};
  SocketAddr destination("127.0.0.1:24222");
  SocketServer server(destination, params);
  server.onRequest = [&](SocketServer::Connection &conn) {
    std::vector<uint8_t> msg(conn.request_buffer.data(),
                             conn.request_buffer.data() + conn.request_buffer.size());
    try {
      auto j = nlohmann::json::from_msgpack(msg);
      std::cout << j.dump(2) << std::endl;
      conn.response_buffer = j.dump(2);
      conn.state.insert(SocketServer::Connection::Responding);
      conn.request_buffer.clear();
    } catch (std::exception &) {
      conn.state.insert(SocketServer::Connection::Receiving);
      // skip invalid payload
     }
    //// Stop after single request:
    // server.Stop();
    // isRunning = false;
  };

  std::cout << "Server is running... Press Ctrl+C to stop." << std::endl;
  server.Start();
  while (isRunning)
  {
    std::this_thread::yield();
  };
  std::cout << "Server stopped." << std::endl;
}
#endif
