cmake_minimum_required(VERSION 3.12)

option(WITH_EXAMPLES "Build examples" ON)

project(opentelemetry-geneva-trace-log-exporter)

include_directories(include)

if(WITH_EXAMPLES)
  add_subdirectory(example)
endif()