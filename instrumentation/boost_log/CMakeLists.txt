# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.12)

set(this_target opentelemetry_boost_log_sink)

project(${this_target})

include(FetchContent)

FetchContent_Declare(
  opentelemetry-cpp
  GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-cpp.git
  GIT_TAG b9cf499ff5715433848b316059714b5c59af1f2c # v1.21.0
)

FetchContent_MakeAvailable(opentelemetry-cpp)

find_package(Boost 1.73 COMPONENTS log REQUIRED)

add_library(${this_target} src/sink.cc)

target_compile_options(${this_target} PUBLIC
  -Wall -Wextra -Werror -Wpedantic -fPIC
  -Wno-error=deprecated-declarations
)

target_include_directories(${this_target} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${this_target} PUBLIC 
  opentelemetry-cpp::api
)

target_link_libraries(${this_target} PRIVATE
  Boost::log
)

if(OPENTELEMETRY_INSTALL)
  include(GNUInstallDirs)

  install(
    TARGETS ${this_target}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(
    DIRECTORY include/opentelemetry/instrumentation/boost_log
    DESTINATION include/opentelemetry/instrumentation
    FILES_MATCHING
    PATTERN "*.h")
endif() # OPENTELEMETRY_INSTALL

if(BUILD_TESTING)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 15460959cbbfa20e66ef0b5ab497367e47fc0a04 # v1.12.0
  )

  FetchContent_MakeAvailable(googletest)

  enable_testing()

  set(testname sink_test)

  include(GoogleTest)

  add_executable(${testname} "test/${testname}.cc")

  target_include_directories(${testname} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  )

  target_link_libraries(${testname} PRIVATE
    GTest::gmock
    GTest::gtest
    GTest::gtest_main
    Boost::log
    opentelemetry-cpp::api
    opentelemetry-cpp::ostream_log_record_exporter
    ${this_target}
  )

  gtest_add_tests(
    TARGET ${testname}
    TEST_PREFIX intrumentation.boost_log.
    TEST_LIST ${testname}
  )
endif() # BUILD_TESTING

if(WITH_EXAMPLES)
  set(example_exe otel_sink_example)
  add_executable(${example_exe} example/main.cc)
  set_target_properties(${example_exe} PROPERTIES EXPORT_NAME ${example_exe})

  target_include_directories(${example_exe} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  )

  target_link_libraries(${example_exe} PRIVATE
    opentelemetry-cpp::api
    opentelemetry-cpp::logs
    opentelemetry-cpp::trace
    opentelemetry-cpp::ostream_log_record_exporter
    opentelemetry-cpp::ostream_span_exporter
    Boost::log
    ${this_target}
  )
endif() # WITH_EXAMPLES
