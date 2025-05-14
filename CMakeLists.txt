# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.14)

project(opentelemetry-cpp-contrib VERSION 0.1.0 LANGUAGES CXX)

# silence CMP0169 deprecation of one-arg Populate()
cmake_policy(SET CMP0169 OLD)

# Define options for each component in opentelemetry-cpp-contrib
option(WITH_GENEVA "Build with Geneva exporters" ON)
option(WITH_USER_EVENTS "Build with User Events exporters" ON)

# Set the opentelemetry-cpp provider to use. 
# - "fetch" will use the FetchContent module to download the opentelemetry-cpp repo and build in the same build tree
# - "package" will find_package to import the installed opentelemetry-cpp package
set(opentelemetry-cpp_PROVIDER "fetch" CACHE STRING "Provider for opentelemetry-cpp")
set_property(CACHE opentelemetry-cpp_PROVIDER PROPERTY STRINGS "package" "fetch")

set(OPENTELEMETRY_INSTALL OFF)
set(OPENTELEMETRY_CONTRIB_INSTALL ON)

include(CTest)
include(GoogleTest)
include(FetchContent)

# Fetch opentelemetry-cpp: 
#   This is required for the cmake install functions. 
FetchContent_Declare(
  opentelemetry-cpp
  GIT_REPOSITORY
  https://github.com/dbarker/opentelemetry-cpp.git
  GIT_TAG
  poc_otel_cmake_external_repo_support
)
# Alternatively fetch from an opentelemetry-cpp submodule
# FetchContent_Declare(
#  opentelemetry-cpp
#  SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/opentelemetry-cpp")

if(opentelemetry-cpp_PROVIDER STREQUAL "package")
  find_package(opentelemetry-cpp CONFIG REQUIRED)
  FetchContent_Populate(opentelemetry-cpp)
elseif(opentelemetry-cpp_PROVIDER STREQUAL "fetch")
  if(WITH_USER_EVENTS)
    # User Events needs the otlp_common component. Turn on one of the otlp components (There is no WITH_OTLP_COMMON option)
    set(WITH_OTLP_FILE ON)
  endif()

  set(OPENTELEMETRY_INSTALL ON)

  # Add opetelemetry-cpp to the build tree
  FetchContent_MakeAvailable(opentelemetry-cpp)

  if(WITH_ABI_VERSION_2)
    set(OPENTELEMETRY_ABI_VERSION_NO "2")
  elseif(WITH_ABI_VERSION_1)
    set(OPENTELEMETRY_ABI_VERSION_NO "1")
  endif()
endif()

# Include the opentelemetry-cpp cmake functions to support adding and installing components
include(${opentelemetry-cpp_SOURCE_DIR}/cmake/otel-install-functions.cmake)

# Add the contrib exporters components to the build tree
add_subdirectory(exporters)

# install the opentelemetry-cpp-contrib package
if(OPENTELEMETRY_CONTRIB_INSTALL)
  # Set the version for the opentelemetry-cpp-contrib package used in the config file template
  set(OPENTELEMETRY_VERSION ${PROJECT_VERSION})
  otel_install_cmake_config()
  otel_install_components()
  otel_install_thirdparty_definitions()
endif() 