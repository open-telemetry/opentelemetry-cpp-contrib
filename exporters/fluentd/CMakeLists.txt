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

# MAIN_PROJECT CHECK
## determine if fluentd exporter is built as a subproject (using add_subdirectory) or if it is the main project
##
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  project(opentelemetry-fluentd)
  set(MAIN_PROJECT ON)
endif()

if (MAIN_PROJECT)
  find_package(opentelemetry-cpp CONFIG QUIET)
  if(opentelemetry-cpp_FOUND)
    message("Using external opentelemetry-cpp")
  else()
    include(cmake/opentelemetry-cpp.cmake)
    build_opentelemetry()
    set(OPENTELEMETRY_CPP_INCLUDE_DIRS "")
    set(OPENTELEMETRY_CPP_LIBRARIES "opentelemetry::libopentelemetry")
    message("opentelemetry-cpp package was not found. Cloned from github")
  endif()
endif()

find_package(nlohmann_json QUIET)
set(nlohmann_json_clone FALSE)
if(nlohmann_json_FOUND)
  message("Using external nlohmann::json")
else()
  include(cmake/nlohmann-json.cmake)
  set(nlohmann_json_clone TRUE)
  set(nlohmann_json_SOURCE_DIR
      "${CMAKE_SOURCE_DIR}/nlohmann_json/single_include")
  include_directories(${nlohmann_json_SOURCE_DIR})
  message("nlohmann_json package was not found. Cloning from github")
endif()

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

include_directories(include)
# create fluentd trace exporter
add_library(opentelemetry_exporter_fluentd_trace src/trace/fluentd_exporter.cc
                                                 src/trace/recordable.cc)
if(MAIN_PROJECT)
  target_include_directories(opentelemetry_exporter_fluentd_trace
                           PRIVATE ${OPENTELEMETRY_CPP_INCLUDE_DIRS})
  target_link_libraries(
    opentelemetry_exporter_fluentd_trace
    PUBLIC ${OPENTELEMETRY_CPP_LIBRARIES}
    INTERFACE nlohmann_json::nlohmann_json)
else()
  target_link_libraries(
    opentelemetry_exporter_fluentd_trace
    PUBLIC opentelemetry_trace opentelemetry_resources opentelemetry_common
    INTERFACE nlohmann_json::nlohmann_json)
endif()

set_target_properties(opentelemetry_exporter_fluentd_trace
                      PROPERTIES EXPORT_NAME trace)

# create fluentd logs exporter

add_library(opentelemetry_exporter_fluentd_logs src/log/fluentd_exporter.cc
                                                src/log/recordable.cc)
if(MAIN_PROJECT)
  target_include_directories(opentelemetry_exporter_fluentd_logs
                           PRIVATE ${OPENTELEMETRY_CPP_INCLUDE_DIRS})
  target_link_libraries(
    opentelemetry_exporter_fluentd_logs
    PUBLIC ${OPENTELEMETRY_CPP_LIBRARIES}
    INTERFACE nlohmann_json::nlohmann_json)
else()
    target_link_libraries(
      opentelemetry_exporter_fluentd_logs
      PUBLIC opentelemetry_logs opentelemetry_resources opentelemetry_common
      INTERFACE nlohmann_json::nlohmann_json)
endif()
set_target_properties(opentelemetry_exporter_fluentd_logs PROPERTIES EXPORT_NAME
                                                                     logs)
if(nlohmann_json_clone)
  add_dependencies(opentelemetry_exporter_fluentd_trace
                   nlohmann_json::nlohmann_json)
  add_dependencies(opentelemetry_exporter_fluentd_logs
                   nlohmann_json::nlohmann_json)
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
  DESTINATION include/opentelemetry/exporters/fluentd/
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

if (MAIN_PROJECT)
  # config file for find_packages(opentelemetry-cpp-fluentd CONFIG)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  set(OPENTELEMETRY_CPP_FLUENTD_VERSION "1.1.2")
  set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}")
  configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/opentelemetry-cpp-fluentd-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    # PATH_VARS OPENTELEMETRY_CPP_FLUENTD_VERSION PROJECT_NAME INCLUDE_INSTALL_DIR
    # CMAKE_INSTALL_LIBDIR
    PATH_VARS PROJECT_NAME INCLUDE_INSTALL_DIR CMAKE_INSTALL_LIBDIR
    NO_CHECK_REQUIRED_COMPONENTS_MACRO)

  # Write version file for find_packages(opentelemetry-cpp-fluentd CONFIG)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config-version.cmake"
    VERSION ${OPENTELEMETRY_CPP_FLUENTD_VERSION}
    COMPATIBILITY ExactVersion)

  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

  # Export all components
  export(
    EXPORT "${PROJECT_NAME}-target"
    NAMESPACE "${PROJECT_NAME}::"
    FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-target.cmake"
  )

  install(
    EXPORT "${PROJECT_NAME}-target"
    NAMESPACE "${PROJECT_NAME}::"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
    
endif()
