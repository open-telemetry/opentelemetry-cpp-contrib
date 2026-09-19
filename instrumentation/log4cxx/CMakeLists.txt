# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

set(this_target opentelemetry_log4cxx_appender)

set(MAIN_PROJECT OFF)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MAIN_PROJECT ON)
  project(${this_target})
  message(STATUS "${PROJECT_NAME} is main project")
endif()

include(FetchContent)

if(MAIN_PROJECT)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake")
  include(opentelemetry-cpp)
endif()

find_package(log4cxx QUIET)

if (log4cxx_FOUND)
  message(STATUS "Found log4cxx version: ${log4cxx_VERSION}")
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LOG4CXX REQUIRED liblog4cxx)
endif()

add_library(${this_target} src/appender.cc)

target_compile_options(${this_target} PUBLIC
  -Wall -Wextra -Werror -Wpedantic -fPIC
  -Wno-error=deprecated-declarations
)

set_target_properties(${this_target} PROPERTIES EXPORT_NAME ${this_target})

target_include_directories(${this_target} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<TARGET_PROPERTY:log4cxx,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(${this_target} PUBLIC 
  opentelemetry-cpp::api
)

target_link_libraries(${this_target} PRIVATE
  log4cxx
)

if(OPENTELEMETRY_INSTALL)
  if(MAIN_PROJECT)
    include(GNUInstallDirs)

    install(
      TARGETS ${this_target}
      EXPORT "${PROJECT_NAME}-target"
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(
      DIRECTORY include/opentelemetry/instrumentation/log4cxx
      DESTINATION include/opentelemetry/instrumentation
      FILES_MATCHING
      PATTERN "*.h")
  else()
    otel_add_component(
      COMPONENT instrumentation_log4cxx
      TARGETS ${this_target}
      FILES_DIRECTORY "include/opentelemetry/instrumentation/log4cxx"
      FILES_DESTINATION "include/opentelemetry/instrumentation"
      FILES_MATCHING
      PATTERN "*.h")
  endif()
endif() # OPENTELEMETRY_INSTALL

if(BUILD_TESTING)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 15460959cbbfa20e66ef0b5ab497367e47fc0a04 # v1.12.0
  )

  FetchContent_MakeAvailable(googletest)

  enable_testing()

  set(testname appender_test)

  include(GoogleTest)

  add_executable(${testname} "test/${testname}.cc")

  target_include_directories(${testname} PUBLIC
    $<TARGET_PROPERTY:log4cxx,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_LIST_DIR}/include
  )

  target_link_libraries(${testname} PRIVATE
    GTest::gmock
    GTest::gtest
    GTest::gtest_main
    log4cxx
    opentelemetry-cpp::ostream_log_record_exporter
    ${this_target}
  )

  gtest_add_tests(
    TARGET ${testname}
    TEST_PREFIX intrumentation.log4cxx.
    TEST_LIST ${testname}
  )
endif() # BUILD_TESTING

if(WITH_EXAMPLES)
  set(example_exe otel_appender_example)
  add_executable(${example_exe} example/main.cc)
  set_target_properties(${example_exe} PROPERTIES EXPORT_NAME ${example_exe})

  target_include_directories(${example_exe} PUBLIC
    $<TARGET_PROPERTY:log4cxx,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_LIST_DIR}/include
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
