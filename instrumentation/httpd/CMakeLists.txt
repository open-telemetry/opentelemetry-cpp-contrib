cmake_minimum_required(VERSION 3.16)

project(opentelemetry-httpd)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The module is a shared object, so every dependency linked into it must be PIC.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)

# Use an installed opentelemetry-cpp when one is available, otherwise build it as part of
# this project.
find_package(opentelemetry-cpp CONFIG QUIET)
if(NOT opentelemetry-cpp_FOUND)
  include(FetchContent)

  set(OPENTELEMETRY_CPP_TAG
      "v1.28.0"
      CACHE STRING "opentelemetry-cpp git tag to build when none is installed")

  set(WITH_OTLP_GRPC ON CACHE BOOL "" FORCE)
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(WITH_BENCHMARK OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    opentelemetry-cpp
    GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-cpp.git
    GIT_TAG ${OPENTELEMETRY_CPP_TAG}
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(opentelemetry-cpp)
endif()

find_path(APR_INCLUDE_DIR
  NAMES apr.h
  PATHS /opt/homebrew/opt/apr/include/apr-1
  /usr/local/include/apr-1
  /usr/local/include/apr-1.0
  /usr/include/apr-1
  /usr/include/apr-1.0
  /usr/local/apr/include/apr-1
)

IF(NOT EXISTS "${APR_INCLUDE_DIR}/apr.h")
  MESSAGE(FATAL_ERROR "APR include directory ${APR_INCLUDE_DIR} is not correct.")
ENDIF()

find_path(APACHE_INCLUDE_DIR
  NAMES httpd.h
  PATHS /usr/include/apache2
  /usr/local/include/apache2
)

IF(NOT EXISTS "${APACHE_INCLUDE_DIR}/httpd.h")
  MESSAGE(FATAL_ERROR "APACHE_INCLUDE_DIR include directory ${APACHE_INCLUDE_DIR} is not correct.")
ENDIF()

add_library(otel_httpd_module SHARED
  src/otel/mod_otel.cpp
  src/otel/opentelemetry.cpp
)

target_compile_options(otel_httpd_module
  PRIVATE -Wall -Wextra
)

install(TARGETS otel_httpd_module DESTINATION ".")

set_target_properties(otel_httpd_module PROPERTIES
  PREFIX ""
)

target_include_directories(otel_httpd_module
  PRIVATE
  ${APACHE_INCLUDE_DIR}
  ${APR_INCLUDE_DIR}
)

target_link_libraries(otel_httpd_module
  PRIVATE
  opentelemetry-cpp::api
  opentelemetry-cpp::trace
  opentelemetry-cpp::ostream_span_exporter
  opentelemetry-cpp::otlp_grpc_exporter
)
