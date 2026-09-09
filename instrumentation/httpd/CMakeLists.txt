cmake_minimum_required(VERSION 3.16)

project(opentelemetry-httpd)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The module is a shared object, so every dependency linked into it must be PIC.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake")

set(WITH_OTLP_GRPC ON CACHE BOOL "" FORCE)
include(opentelemetry-cpp)

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
