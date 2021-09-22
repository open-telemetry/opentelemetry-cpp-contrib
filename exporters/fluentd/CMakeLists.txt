# Copyright 2021, OpenTelemetry Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.12)

project(opentelemetry-fluentd)

ADD_DEFINITIONS(-DHAVE_CONSOLE_LOG)
ADD_DEFINITIONS(-DENABLE_LOGS_PREVIEW)
find_package(opentelemetry-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)

if(COMMAND cmake_policy)
cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

include_directories(include)

#create fluentd trace exporter
add_library(opentelemetry_exporter_fluentd_trace src/trace/fluentd_exporter.cc
                                                 src/trace/recordable.cc)
target_include_directories(opentelemetry_exporter_fluentd_trace
    PRIVATE
    ${OPENTELEMETRY_CPP_INCLUDE_DIRS})
target_link_libraries(opentelemetry_exporter_fluentd_trace
                      PUBLIC
                      ${OPENTELEMETRY_CPP_LIBRARIES}
                      INTERFACE nlohmann_json::nlohmann_json)

#create fluentd logs exporter

add_library(opentelemetry_exporter_fluentd_logs src/log/fluentd_exporter.cc
                                                 src/log/recordable.cc)
target_include_directories(opentelemetry_exporter_fluentd_logs
    PRIVATE
    ${OPENTELEMETRY_CPP_INCLUDE_DIRS})
target_link_libraries(opentelemetry_exporter_fluentd_logs
                      PUBLIC
                      ${OPENTELEMETRY_CPP_LIBRARIES}
                      INTERFACE nlohmann_json::nlohmann_json)

add_subdirectory(example)

install(
  TARGETS opentelemetry_exporter_fluentd_trace
  EXPORT "${PROJECT_NAME}-target"
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY include/opentelemetry/exporters/fluentd/
  DESTINATION include/opentelemetry/exporters/
  FILES_MATCHING
  PATTERN "*.h")

if(BUILD_TESTING)
  add_executable(fluentd_recordable_trace_test test/trace/fluentd_recordable_test.cc test/trace/fluentd_baseline_test.cc)

  target_link_libraries(
    fluentd_recordable_trace_test
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    opentelemetry_common
    opentelemetry_trace
    opentelemetry_resources
    opentelemetry_sdk
    opentelemetry_exporter_fluentd_trace)

  gtest_add_tests(
    TARGET fluentd_recordable_trace_test
    TEST_PREFIX exporter.
    TEST_LIST fluentd_recordable_trace_test)
endif() # BUILD_TESTING
