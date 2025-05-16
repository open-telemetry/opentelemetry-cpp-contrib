# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.14)

project(opentelemetry-cpp-contrib VERSION 0.1.0 LANGUAGES CXX)

# Define options for each component in opentelemetry-cpp-contrib
option(WITH_COMPONENT_GENEVA "Build with Geneva exporters" ON)
option(WITH_COMPONENT_USER_EVENTS "Build with User Events exporters" ON)

# Other options for opentelemetry-cpp-contrib
option(OPENTELEMETRY_CONTRIB_INSTALL "Install the opentelemetry-cpp-contrib package" ON)
option(WITH_EXAMPLES "Build example" ON)
option(WITH_BENCHMARK "Build with benchmark" ON)
option(BUILD_TESTING "Build tests" ON)
option(BUILD_TRACEPOINTS "Build tracepoints library" ON)
option(OTELCPP_VERSIONED_LIBS "Whether to generate the versioned shared libs" OFF)

# Set the opentelemetry-cpp provider to use. 
# - "fetch" will use the FetchContent module to download the opentelemetry-cpp repo and build in the same build tree
# - "package" will find_package to import the installed opentelemetry-cpp package
set(opentelemetry-cpp_PROVIDER "fetch" CACHE STRING "Provider for opentelemetry-cpp")
set_property(CACHE opentelemetry-cpp_PROVIDER PROPERTY STRINGS "package" "fetch")

include(FetchContent)

if(BUILD_TESTING)
  include(CTest)
  include(GoogleTest)
  # Find or Fetch googletest
  include(${PROJECT_SOURCE_DIR}/cmake/googletest.cmake)
  enable_testing()
endif() 

if(WITH_BENCHMARK)
  # Find or Fetch benchmark
  include(${PROJECT_SOURCE_DIR}/cmake/benchmark.cmake)
endif() 

# Make Threads available
find_package(Threads REQUIRED)

# Find or Fetch opentelemetry-cpp
include(${PROJECT_SOURCE_DIR}/cmake/opentelemetry-cpp.cmake)

# Include the opentelemetry-cpp cmake functions to support adding and installing components
include(${opentelemetry-cpp_SOURCE_DIR}/cmake/otel-install-functions.cmake)

# Add the contrib exporters components to the build tree
add_subdirectory(exporters)

# install the opentelemetry-cpp-contrib package
if(OPENTELEMETRY_CONTRIB_INSTALL)
  otel_install_cmake_config()
  otel_install_components()
  otel_install_thirdparty_definitions()
endif() 