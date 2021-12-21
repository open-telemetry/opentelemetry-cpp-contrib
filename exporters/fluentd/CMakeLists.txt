# Copyright 2021, OpenTelemetry Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

cmake_minimum_required(VERSION 3.12)

project(opentelemetry-fluentd)

add_definitions(-DHAVE_CONSOLE_LOG)
add_definitions(-DENABLE_LOGS_PREVIEW)
find_package(opentelemetry-cpp REQUIRED)
find_package(nlohmann_json QUIET)
set(nlohmann_json_clone FALSE)
if(nlohmann_json_FOUND)
  message("Using external nlohmann::json")
else()
  include(cmake/nlohmann-json.cmake)
  set(nlohmann_json_clone TRUE)
  message("\nnlohmann_json package was not found. Cloning from github")
endif()

find_package(CURL REQUIRED)

include_directories(include)
# create fluentd trace exporter
add_library(opentelemetry_exporter_fluentd_trace src/trace/fluentd_exporter.cc
                                                 src/trace/recordable.cc)
target_include_directories(opentelemetry_exporter_fluentd_trace
                           PRIVATE ${OPENTELEMETRY_CPP_INCLUDE_DIRS})
target_link_libraries(
  opentelemetry_exporter_fluentd_trace
  PUBLIC ${OPENTELEMETRY_CPP_LIBRARIES}
  INTERFACE nlohmann_json::nlohmann_json)

# create fluentd logs exporter

add_library(opentelemetry_exporter_fluentd_logs src/log/fluentd_exporter.cc
                                                src/log/recordable.cc)
target_include_directories(opentelemetry_exporter_fluentd_logs
                           PRIVATE ${OPENTELEMETRY_CPP_INCLUDE_DIRS})
target_link_libraries(
  opentelemetry_exporter_fluentd_logs
  PUBLIC ${OPENTELEMETRY_CPP_LIBRARIES}
  INTERFACE nlohmann_json::nlohmann_json)

if(nlohmann_json_clone)
  add_dependencies(opentelemetry_exporter_fluentd_trace nlohmann_json::nlohmann_json)
  add_dependencies(opentelemetry_exporter_fluentd_logs nlohmann_json::nlohmann_json)
  include_directories(${PROJECT_BINARY_DIR}/include)
endif()
  
add_subdirectory(example)

install(
  TARGETS opentelemetry_exporter_fluentd_trace
  EXPORT "${PROJECT_NAME}-target"
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  TARGETS opentelemetry_exporter_fluentd_logs
  EXPORT "${PROJECT_NAME}-target"
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY include/opentelemetry/exporters/fluentd/
  DESTINATION include/opentelemetry/exporters/fluentd
  FILES_MATCHING
  PATTERN "*.h")

if(BUILD_TESTING)
  include(GoogleTest)
  # build trace exporter tests
  add_executable(
    fluentd_recordable_trace_test test/trace/fluentd_recordable_test.cc
                                  test/trace/fluentd_baseline_test.cc)

  target_link_libraries(
    fluentd_recordable_trace_test
    gtest
    gtest_main
    ${CMAKE_THREAD_LIBS_INIT}
    opentelemetry_common
    opentelemetry_trace
    opentelemetry_resources
    opentelemetry_exporter_fluentd_trace)

  if(nlohmann_json_clone)
    add_dependencies(fluentd_recordable_trace_test nlohmann_json::nlohmann_json)
  endif()

  gtest_add_tests(
    TARGET fluentd_recordable_trace_test
    TEST_PREFIX exporter.
    TEST_LIST fluentd_recordable_trace_test)

  # build log exporter tests
  add_executable(fluentd_recordable_logs_test
                 test/log/fluentd_recordable_test.cc)

  target_link_libraries(
    fluentd_recordable_logs_test
    gtest
    gtest_main
    ${CMAKE_THREAD_LIBS_INIT}
    opentelemetry_common
    opentelemetry_logs
    opentelemetry_resources
    opentelemetry_exporter_fluentd_logs)

  if(nlohmann_json_clone)
      add_dependencies(fluentd_recordable_logs_test nlohmann_json::nlohmann_json)
    endif()
    
  gtest_add_tests(
    TARGET fluentd_recordable_logs_test
    TEST_PREFIX exporter.
    TEST_LIST fluentd_recordable_logs_test)
endif() # BUILD_TESTING
