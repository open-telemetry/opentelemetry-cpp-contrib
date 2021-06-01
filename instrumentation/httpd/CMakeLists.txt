cmake_minimum_required(VERSION 3.12)

project(opentelemetry-httpd)

find_package(opentelemetry-cpp REQUIRED)
find_package(Threads REQUIRED)
find_package(protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(CURL REQUIRED)

add_library(otel_httpd_module SHARED
  src/otel/mod_otel.cpp
  src/otel/opentelemetry.cpp
)

target_compile_options(otel_httpd_module
  PRIVATE -Wall -Wextra -fpermissive
)

install(TARGETS otel_httpd_module DESTINATION ".")

set_target_properties(otel_httpd_module PROPERTIES
  PREFIX ""
)

target_include_directories(otel_httpd_module
  PRIVATE
  ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
  "/usr/include/apache2"
  "/usr/include/apr-1.0"
)

target_link_libraries(otel_httpd_module
  PRIVATE
  ${OPENTELEMETRY_CPP_LIBRARIES}
  gRPC::grpc++
#  Thrift::thrift
)
