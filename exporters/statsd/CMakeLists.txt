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
## determine if statsd exporter is built as a subproject (using add_subdirectory) or if it is the main project
##
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  project(opentelemetry-cpp-statsd)
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


include_directories(include)

# create statsd metrics exporter
if(WIN32)
  add_library(
    opentelemetry_exporter_statsd_metrics
    src/exporter.cc src/etw_data_transport.cc
    src/socket_data_transport.cc)
else()
  add_library(opentelemetry_exporter_statsd_metrics
              src/exporter.cc src/socket_data_transport.cc)
endif()

if(MAIN_PROJECT)
  target_include_directories(opentelemetry_exporter_statsd_metrics
                           PRIVATE ${OPENTELEMETRY_CPP_INCLUDE_DIRS})
  target_link_libraries(
    opentelemetry_exporter_statsd_metrics
    PUBLIC ${OPENTELEMETRY_CPP_LIBRARIES}
    )
  set_target_properties(opentelemetry_exporter_statsd_metrics
                        PROPERTIES EXPORT_NAME metrics)
else()
  target_link_libraries(
    opentelemetry_exporter_statsd_metrics
    PUBLIC opentelemetry_trace opentelemetry_resources opentelemetry_common
    )
endif()


if(MAIN_PROJECT)
  option(WITH_EXAMPLES "Build examples" ON)
endif()

if (WITH_EXAMPLES)
  add_subdirectory(example)
endif()

if(OPENTELEMETRY_INSTALL)
  install(
    TARGETS opentelemetry_exporter_statsd_metrics
    EXPORT "${PROJECT_NAME}-target"
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  install(
    DIRECTORY include/opentelemetry/exporters/
    DESTINATION include/opentelemetry/exporters/
    FILES_MATCHING
    PATTERN "*.h")
endif()

if(BUILD_TESTING)
  if(MAIN_PROJECT)
    find_package(GTest CONFIG REQUIRED)
  else()
    if (NOT DEFINED GTEST_BOTH_LIBRARIES)
      message(STATUS_FATAL, "Test is not enable.")
    endif()
  endif()

endif() # BUILD_TESTING

if (MAIN_PROJECT)
  # config file for find_packages(opentelemetry-cpp-statsd CONFIG)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  set(OPENTELEMETRY_CPP_STATSD_VERSION "1.0.0")
  set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}")
  configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/opentelemetry-cpp-statsd-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    # PATH_VARS OPENTELEMETRY_CPP_STATSD_VERSION PROJECT_NAME INCLUDE_INSTALL_DIR
    # CMAKE_INSTALL_LIBDIR
    PATH_VARS PROJECT_NAME INCLUDE_INSTALL_DIR CMAKE_INSTALL_LIBDIR
    NO_CHECK_REQUIRED_COMPONENTS_MACRO)

  # Write version file for find_packages(opentelemetry-cpp-statsd CONFIG)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config-version.cmake"
    VERSION ${OPENTELEMETRY_CPP_STATSD_VERSION}
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
