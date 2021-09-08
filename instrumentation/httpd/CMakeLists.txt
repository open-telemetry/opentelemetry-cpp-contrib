cmake_minimum_required(VERSION 3.12)

project(opentelemetry-httpd)

find_package(opentelemetry-cpp REQUIRED)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(CURL REQUIRED)
find_package(Thrift REQUIRED)

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
  ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
  ${APACHE_INCLUDE_DIR}
  ${APR_INCLUDE_DIR}
)

target_link_libraries(otel_httpd_module
  PRIVATE
  ${OPENTELEMETRY_CPP_LIBRARIES}
  gRPC::grpc++
#  Thrift::thrift
)
