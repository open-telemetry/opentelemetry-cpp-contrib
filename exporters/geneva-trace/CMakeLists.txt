cmake_minimum_required(VERSION 3.16)

# MAIN_PROJECT CHECK
# determine if this project is built as a subproject (using add_subdirectory) or if this is the main project
set(MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  project(opentelemetry-geneva-trace-log-exporter)
  set(MAIN_PROJECT ON)
endif()

include(FetchContent)

if(MAIN_PROJECT)
  option(WITH_EXAMPLES "Build examples" ON)

  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake")

  include(opentelemetry-cpp)
endif()

add_library(opentelemetry_exporter_geneva_trace INTERFACE)
target_include_directories(
  opentelemetry_exporter_geneva_trace INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
set_target_properties(
  opentelemetry_exporter_geneva_trace
  PROPERTIES EXPORT_NAME opentelemetry_exporter_geneva_trace)

add_library(opentelemetry_exporter_geneva_logs INTERFACE)
target_include_directories(
  opentelemetry_exporter_geneva_logs INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
set_target_properties(
  opentelemetry_exporter_geneva_logs
  PROPERTIES EXPORT_NAME opentelemetry_exporter_geneva_logs)

target_link_libraries(opentelemetry_exporter_geneva_logs
                      PUBLIC opentelemetry-cpp::trace opentelemetry-cpp::resources opentelemetry-cpp::common)

if(WITH_EXAMPLES)
  include_directories(include)
  add_subdirectory(example)
endif()

if(OPENTELEMETRY_INSTALL)
  if(MAIN_PROJECT)
    install(DIRECTORY include/ DESTINATION include)

    install(
      TARGETS opentelemetry_exporter_geneva_trace opentelemetry_exporter_geneva_logs
      EXPORT "${PROJECT_NAME}-target")

    if(NOT MAIN_PROJECT)
      install(
        EXPORT "${PROJECT_NAME}-target"
        NAMESPACE "${PROJECT_NAME}::"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
    endif()
  else()
    otel_add_component(
      COMPONENT
      exporters_geneva_trace_logs
      TARGETS
      opentelemetry_exporter_geneva_trace
      opentelemetry_exporter_geneva_logs
      FILES_DIRECTORY
      "include/opentelemetry/exporters/geneva"
      FILES_DESTINATION
      "include/opentelemetry/exporters"
      FILES_MATCHING
      PATTERN
      "*.h")
  endif()
endif()
