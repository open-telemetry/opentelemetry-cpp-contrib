# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.12)

set(this_target opentelemetry_log4cxx_appender)

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
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<TARGET_PROPERTY:log4cxx,INTERFACE_INCLUDE_DIRECTORIES>
  ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
)

target_link_libraries(${this_target} PRIVATE
  log4cxx
)

if(OPENTELEMETRY_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

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

  configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    NO_CHECK_REQUIRED_COMPONENTS_MACRO)

  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

  export(
    EXPORT "${PROJECT_NAME}-target"
    NAMESPACE ${PROJECT_NAME}::
    FILE "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-target.cmake")

  install(
    EXPORT "${PROJECT_NAME}-target"
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
endif() # OPENTELEMETRY_INSTALL

if(BUILD_TESTING)
  find_package(GTest 1.12 REQUIRED)

  enable_testing()

  set(testname appender_test)

  include(GoogleTest)

  add_executable(${testname} "test/${testname}.cc")

  target_include_directories(${testname} PUBLIC
    $<TARGET_PROPERTY:log4cxx,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
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
