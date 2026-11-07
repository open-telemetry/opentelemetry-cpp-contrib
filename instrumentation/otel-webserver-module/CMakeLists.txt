cmake_minimum_required(VERSION 3.16)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.properties" _version_props)
foreach(_line IN LISTS _version_props)
    if(_line MATCHES "^server-module-version=(.+)$")
        set(_module_version "${CMAKE_MATCH_1}")
    endif()
endforeach()

project(otel-webserver-module VERSION ${_module_version} LANGUAGES CXX)

option(OTEL_WEBSERVER_BUILD_APACHE "Build the Apache httpd modules" ON)
option(OTEL_WEBSERVER_BUILD_TESTS  "Build the unit tests"           ON)
option(OTEL_ENABLE_COVERAGE        "Instrument for gcov/lcov"       OFF)

# Pinned deliberately.
# TODO: bump version following otel-cpp
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(OTEL_ARCH_FLAG "-m64" CACHE STRING "Target architecture flag")

find_package(Threads REQUIRED)
include(cmake/otel-webserver-deps.cmake)

if(OTEL_ENABLE_COVERAGE)
    add_compile_options(-g -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
endif()

add_subdirectory(src)

if(OTEL_WEBSERVER_BUILD_APACHE)
    add_subdirectory(src/apache)
endif()

if(OTEL_WEBSERVER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test/unit)
endif()
