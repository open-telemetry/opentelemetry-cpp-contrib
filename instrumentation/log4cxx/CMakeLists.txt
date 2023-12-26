# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.12)

set(this_target opentelemetry_log4cxx)

project(${this_target})

find_package(opentelemetry-cpp QUIET)

if (opentelemetry-cpp_FOUND)
  message(STATUS "Found opentelemetry-cpp version: ${opentelemetry-cpp_VERSION}")
else()
  message(FATAL_ERROR "Could not find opentelemetry-cpp")
endif()

find_package(log4cxx QUIET)

if (log4cxx_FOUND)
  message(STATUS "Found log4cxx version: ${log4cxx_VERSION}")
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LOG4CXX REQUIRED liblog4cxx)
endif()

add_library(${this_target} src/appender.cc)

target_compile_options(${this_target}
  PUBLIC -Wall -Wextra -Werror -Wpedantic -fPIC
)

set_target_properties(${this_target} PROPERTIES EXPORT_NAME ${this_target})

target_include_directories(${this_target} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  $<TARGET_PROPERTY:log4cxx,INTERFACE_INCLUDE_DIRECTORIES>
  ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
)

target_link_libraries(${this_target} PRIVATE
  log4cxx
)

if(OPENTELEMETRY_INSTALL)
  install(
    TARGETS ${this_target}
    EXPORT "${PROJECT_NAME}-target"
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  install(
    DIRECTORY include/opentelemetry/instrumentation/log4cxx
    DESTINATION include/opentelemetry/instrumentation
    FILES_MATCHING
    PATTERN "*.h")
endif() # OPENTELEMETRY_INSTALL

if(BUILD_TESTING)
  include(GoogleTest)
  
  foreach(testname appender_test)
    add_executable(${testname} "test/${testname}.cc")

    target_include_directories(${testname} PUBLIC
      $<TARGET_PROPERTY:log4cxx,INTERFACE_INCLUDE_DIRECTORIES>
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
    )

    target_link_libraries(${testname}
      gtest
      gtest_main
      log4cxx
      ${this_target})

    gtest_add_tests(
      TARGET ${testname}
      TEST_PREFIX intrumentation.log4cxx.
      TEST_LIST ${testname})
  endforeach()
endif() # BUILD_TESTING

if(WITH_EXAMPLES)
  set(example_exe otel_appender_example)
  add_executable(${example_exe} example/main.cc)
  set_target_properties(${example_exe} PROPERTIES EXPORT_NAME ${example_exe})

  target_include_directories(${example_exe} PUBLIC
    $<TARGET_PROPERTY:log4cxx,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
  )

  target_link_libraries(${example_exe} PRIVATE
    opentelemetry-cpp::logs
    opentelemetry-cpp::trace
    opentelemetry-cpp::ostream_log_record_exporter
    opentelemetry-cpp::ostream_span_exporter
    log4cxx
    ${this_target}
  )
endif() # WITH_EXAMPLES
