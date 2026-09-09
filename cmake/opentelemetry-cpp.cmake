# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

if(NOT opentelemetry-cpp_GIT_TAG)
  set(opentelemetry-cpp_GIT_TAG
      "v1.28.0"
      CACHE STRING "opentelemetry-cpp git tag to build when none is installed")
endif()

find_package(opentelemetry-cpp CONFIG QUIET)
set(opentelemetry-cpp_PROVIDER "find_package")

if(NOT opentelemetry-cpp_FOUND)
  include(FetchContent)

  set(WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(WITH_BENCHMARK OFF CACHE BOOL "" FORCE)

  set(_otel_cpp_prev_build_testing "${BUILD_TESTING}")
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    opentelemetry-cpp
    GIT_REPOSITORY "https://github.com/open-telemetry/opentelemetry-cpp.git"
    GIT_TAG "${opentelemetry-cpp_GIT_TAG}"
    GIT_SHALLOW TRUE)
  set(opentelemetry-cpp_PROVIDER "fetch_repository")

  FetchContent_MakeAvailable(opentelemetry-cpp)

  if(_otel_cpp_prev_build_testing STREQUAL "")
    unset(BUILD_TESTING CACHE)
  else()
    set(BUILD_TESTING "${_otel_cpp_prev_build_testing}" CACHE BOOL "" FORCE)
  endif()
  unset(_otel_cpp_prev_build_testing)

  string(REGEX REPLACE "^v([0-9]+\\.[0-9]+\\.[0-9]+)$" "\\1" opentelemetry-cpp_VERSION
                       "${opentelemetry-cpp_GIT_TAG}")
endif()

if(NOT TARGET opentelemetry-cpp::api)
  message(FATAL_ERROR "A required opentelemetry-cpp target (opentelemetry-cpp::api) was not imported")
endif()

message(STATUS "opentelemetry-cpp: ${opentelemetry-cpp_VERSION} (${opentelemetry-cpp_PROVIDER})")
