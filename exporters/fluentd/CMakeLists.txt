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

cmake_minimum_required(VERSION 3.16)

# MAIN_PROJECT CHECK
## determine if fluentd exporter is built as a subproject (using add_subdirectory) or if it is the main project
##
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  project(opentelemetry-cpp-fluentd)
  set(MAIN_PROJECT ON)
endif()

include(FetchContent)

if (MAIN_PROJECT)
  FetchContent_Declare(
    opentelemetry-cpp
    GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-cpp.git
    GIT_TAG b9cf499ff5715433848b316059714b5c59af1f2c # v1.21.0
  )

  FetchContent_MakeAvailable(opentelemetry-cpp)
endif()

find_package(nlohmann_json QUIET)

if(NOT nlohmann_json_FOUND)
  FetchContent_Declare(
    glog
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG db78ac1d7716f56fc9f1b030b715f872f93964e4 # v3.9.1
  )

  FetchContent_MakeAvailable(glog)
endif()

if(MAIN_PROJECT)
  find_package(CURL REQUIRED)
  find_package(Threads REQUIRED)
endif()

include_directories(include)

if(OPENTELEMETRY_ENABLE_FLUENT_RESOURCE_PUBLISH_EXPERIMENTAL)
  add_definitions(-DOPENTELEMETRY_ENABLE_FLUENT_RESOURCE_PUBLISH_EXPERIMENTAL)
endif()

# create fluentd trace exporter
add_library(opentelemetry_exporter_geneva_trace src/trace/fluentd_exporter.cc
                                                src/trace/recordable.cc)
set_target_properties(
  opentelemetry_exporter_geneva_trace
  PROPERTIES EXPORT_NAME opentelemetry_exporter_geneva_trace)

if(MAIN_PROJECT)
  target_link_libraries(
    opentelemetry_exporter_geneva_trace
    PUBLIC opentelemetry-cpp::api)
else()
  target_link_libraries(
    opentelemetry_exporter_geneva_trace
    PUBLIC opentelemetry_trace opentelemetry_resources opentelemetry_common
      opentelemetry_ext)
endif()

# create fluentd logs exporter

add_library(opentelemetry_exporter_geneva_logs src/log/fluentd_exporter.cc
                                               src/log/recordable.cc)
set_target_properties(
  opentelemetry_exporter_geneva_logs
  PROPERTIES EXPORT_NAME opentelemetry_exporter_geneva_logs)

if(MAIN_PROJECT)
  target_link_libraries(
    opentelemetry_exporter_geneva_logs
    PUBLIC opentelemetry-cpp::api)
else()
  target_link_libraries(
    opentelemetry_exporter_geneva_logs
    PUBLIC opentelemetry_logs opentelemetry_resources opentelemetry_common
      opentelemetry_ext)
endif()

if(MAIN_PROJECT)
  option(WITH_EXAMPLES "Build examples" ON)
endif()

if (WITH_EXAMPLES)
  add_subdirectory(example)
endif()

if(OPENTELEMETRY_INSTALL)
  if(MAIN_PROJECT)
    include(GNUInstallDirs)

    install(
      TARGETS opentelemetry_exporter_geneva_logs
              opentelemetry_exporter_geneva_trace
      EXPORT "${PROJECT_NAME}-target"
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

    install(
      DIRECTORY include/opentelemetry/exporters/
      DESTINATION include/opentelemetry/exporters/
      FILES_MATCHING
      PATTERN "*.h")
  else()
    otel_add_component(
      COMPONENT exporters_geneva_fluentd
      TARGETS opentelemetry_exporter_geneva_logs opentelemetry_exporter_geneva_trace
      FILES_DIRECTORY "include/opentelemetry/exporters/"
      FILES_DESTINATION "include/opentelemetry/exporters"
      FILES_MATCHING
      PATTERN "*.h")
  endif()
endif()

if(BUILD_TESTING)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 15460959cbbfa20e66ef0b5ab497367e47fc0a04 # v1.12.0
  )

  FetchContent_MakeAvailable(googletest)

  enable_testing()

  include(GoogleTest)

  # build trace exporter tests
  add_executable(
    fluentd_recordable_trace_test test/trace/fluentd_recordable_test.cc
                                  test/trace/fluentd_baseline_test.cc)

  target_link_libraries(
    fluentd_recordable_trace_test
    GTest::gmock
    GTest::gtest
    GTest::gtest_main
    ${CMAKE_THREAD_LIBS_INIT}
    opentelemetry_common
    opentelemetry_trace
    opentelemetry_resources
    opentelemetry_exporter_geneva_trace)

  gtest_add_tests(
    TARGET fluentd_recordable_trace_test
    TEST_PREFIX exporter.
    TEST_LIST fluentd_recordable_trace_test)

  # build log exporter tests
  add_executable(fluentd_recordable_logs_test
                 test/log/fluentd_recordable_test.cc)

  target_link_libraries(
    fluentd_recordable_logs_test
    GTest::gmock
    GTest::gtest
    GTest::gtest_main
    ${CMAKE_THREAD_LIBS_INIT}
    opentelemetry_common
    opentelemetry_logs
    opentelemetry_resources
    opentelemetry_exporter_geneva_logs)

  gtest_add_tests(
    TARGET fluentd_recordable_logs_test
    TEST_PREFIX exporter.
    TEST_LIST fluentd_recordable_logs_test)
endif() # BUILD_TESTING
