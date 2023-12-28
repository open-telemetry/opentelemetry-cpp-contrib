# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.12)

set(this_target opentelemetry_spdlog)

project(${this_target})

find_package(opentelemetry-cpp REQUIRED)
find_package(spdlog REQUIRED)

add_library(${this_target} src/sink.cc)

target_compile_options(${this_target} PUBLIC
  -Wall -Wextra -Werror -Wpedantic -fPIC
)

set_target_properties(${this_target} PROPERTIES EXPORT_NAME ${this_target})

target_include_directories(${this_target} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
)

target_link_libraries(${this_target} PRIVATE
  spdlog::spdlog
)

if(OPENTELEMETRY_INSTALL)
  install(
    TARGETS ${this_target}
    EXPORT "${PROJECT_NAME}-target"
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  install(
    DIRECTORY include/opentelemetry/instrumentation/spdlog
    DESTINATION include/opentelemetry/instrumentation
    FILES_MATCHING
    PATTERN "*.h")
endif() # OPENTELEMETRY_INSTALL

if(BUILD_TESTING)
  set(testname sink_test)

  include(GoogleTest)

  add_executable(${testname} "test/${testname}.cc")

  target_include_directories(${testname} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
  )

  target_link_libraries(${testname} PRIVATE
    gmock
    gtest
    gtest_main
    spdlog::spdlog
    opentelemetry-cpp::ostream_log_record_exporter
    ${this_target}
  )

  gtest_add_tests(
    TARGET ${testname}
    TEST_PREFIX intrumentation.spdlog.
    TEST_LIST ${testname}
  )
endif() # BUILD_TESTING

if(WITH_EXAMPLES)
  set(example_exe otel_sink_example)
  add_executable(${example_exe} example/main.cc)
  set_target_properties(${example_exe} PROPERTIES EXPORT_NAME ${example_exe})

  target_include_directories(${example_exe} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
  )

  target_link_libraries(${example_exe} PRIVATE
    opentelemetry-cpp::logs
    opentelemetry-cpp::trace
    opentelemetry-cpp::ostream_log_record_exporter
    opentelemetry-cpp::ostream_span_exporter
    spdlog::spdlog
    ${this_target}
  )
endif() # WITH_EXAMPLES
