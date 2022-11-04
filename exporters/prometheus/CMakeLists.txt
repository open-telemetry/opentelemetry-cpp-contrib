# Copyright 2022, OpenTelemetry Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

cmake_minimum_required(VERSION 3.12)

project(
  opentelemetry-cpp-contrib
  VERSION "1.6.1" # opentelemetry-cpp-contrib 1.6.1
  HOMEPAGE_URL "https://github.com/open-telemetry/opentelemetry-cpp-contrib"
  LANGUAGES CXX)

if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
      "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
endif()

if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
      "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
endif()

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
      "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
endif()

link_directories(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

find_package(opentelemetry-cpp REQUIRED)
find_package(prometheus-cpp CONFIG REQUIRED)

get_target_property(OPENTELEMETRY_CPP_API_DEFINITIONS opentelemetry-cpp::api
                    INTERFACE_COMPILE_DEFINITIONS)
if(HAVE_ABSEIL IN_LIST OPENTELEMETRY_CPP_API_DEFINITIONS)
  find_package(absl REQUIRED)
endif()

if(HAVE_GSL IN_LIST OPENTELEMETRY_CPP_API_DEFINITIONS)
  find_package(Microsoft.GSL REQUIRED)
endif()

include_directories(include)
include(CMakeDependentOption)

cmake_dependent_option(BUILD_TESTING "Enable tests" ON
                       "NOT CMAKE_CROSSCOMPILING" OFF)

if(BUILD_TESTING)
  enable_testing()
  add_definitions(-DENABLE_TEST)
  find_package(GTest)
  include(GoogleTest)

  if(TARGET GTest::gtest)
    set(OPENTELEMETRY_CPP_CONTRIB_GTEST_LINK_LIB_NAME GTest::gtest)
  elseif(TARGET GTest::GTest)
    set(OPENTELEMETRY_CPP_CONTRIB_GTEST_LINK_LIB_NAME GTest::GTest)
  else()
    set(OPENTELEMETRY_CPP_CONTRIB_GTEST_LINK_LIB_NAME ${GTEST_LIBRARIES})
  endif()

  if(TARGET GTest::gtest_main)
    set(OPENTELEMETRY_CPP_CONTRIB_GTEST_LINK_MAIN_NAME GTest::gtest_main)
  elseif(TARGET GTest::Main)
    set(OPENTELEMETRY_CPP_CONTRIB_GTEST_LINK_MAIN_NAME GTest::Main)
  else()
    set(OPENTELEMETRY_CPP_CONTRIB_GTEST_LINK_MAIN_NAME ${GTEST_MAIN_LIBRARIES})
  endif()
endif()

add_library(opentelemetry_prometheus_push_exporter src/push_exporter.cc)
set_target_properties(opentelemetry_prometheus_push_exporter
                      PROPERTIES EXPORT_NAME prometheus_push_exporter)

target_include_directories(
  opentelemetry_prometheus_push_exporter
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
         "$<INSTALL_INTERFACE:include>")

target_link_libraries(
  opentelemetry_prometheus_push_exporter
  PUBLIC opentelemetry-cpp::prometheus_exporter opentelemetry-cpp::metrics
         opentelemetry-cpp::resources prometheus-cpp::push prometheus-cpp::core)

if(DEFINED VCPKG_CMAKE_SYSTEM_NAME AND VCPKG_CMAKE_SYSTEM_NAME STREQUAL
                                       "WindowsStore")
  set(OPENTELEMETRY_CPP_CONTRIB_TARGET_IS_WINDOWS ON)
elseif(MINGW OR CMAKE_SYSTEM_NAME STREQUAL "MinGW")
  set(OPENTELEMETRY_CPP_CONTRIB_TARGET_IS_WINDOWS ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(OPENTELEMETRY_CPP_CONTRIB_TARGET_IS_WINDOWS ON)
endif()
if(OPENTELEMETRY_CPP_CONTRIB_TARGET_IS_WINDOWS)
  target_link_libraries(
    opentelemetry_prometheus_push_exporter
    PUBLIC advapi32
           bcrypt
           iphlpapi
           psapi
           user32
           wsock32
           userenv
           Ws2_32
           wldap32
           crypt32)
elseif(UNIX OR MINGW)
  set(OPENTELEMETRY_CPP_CONTRIB_TEST_BACKUP_CMAKE_REQUIRED_LIBRARIES
      ${CMAKE_REQUIRED_LIBRARIES})
  set(OPENTELEMETRY_CPP_CONTRIB_TEST_SYSTEM_LIBRARIES m dl pthread rt gcc gcc_s)
  include(CheckCXXSourceCompiles)

  foreach(LIBNAME IN LISTS OPENTELEMETRY_CPP_CONTRIB_TEST_SYSTEM_LIBRARIES)
    if(OPENTELEMETRY_CPP_CONTRIB_TEST_BACKUP_CMAKE_REQUIRED_LIBRARIES)
      set(CMAKE_REQUIRED_LIBRARIES
          "${OPENTELEMETRY_CPP_CONTRIB_TEST_BACKUP_CMAKE_REQUIRED_LIBRARIES};${LIBNAME}"
      )
    else()
      set(CMAKE_REQUIRED_LIBRARIES "${LIBNAME}")
    endif()
    string(TOUPPER "${LIBNAME}" OPENTELEMETRY_CPP_CONTRIB_TEST_NAME)
    set(OPENTELEMETRY_CPP_CONTRIB_TEST_NAME
        "OPENTELEMETRY_CPP_CONTRIB_TEST_LINK_${OPENTELEMETRY_CPP_CONTRIB_TEST_NAME}"
    )
    check_cxx_source_compiles("#include <cstdio>
    int main() { return 0; }" ${OPENTELEMETRY_CPP_CONTRIB_TEST_NAME})
    if(${OPENTELEMETRY_CPP_CONTRIB_TEST_NAME})
      list(APPEND OPENTELEMETRY_CPP_CONTRIB_SYSTEM_LIBRARIES "${LIBNAME}")
    endif()
  endforeach()

  unset(OPENTELEMETRY_CPP_CONTRIB_TEST_NAME)
  if(OPENTELEMETRY_CPP_CONTRIB_TEST_BACKUP_CMAKE_REQUIRED_LIBRARIES)
    set(CMAKE_REQUIRED_LIBRARIES
        "${OPENTELEMETRY_CPP_CONTRIB_TEST_BACKUP_CMAKE_REQUIRED_LIBRARIES}")
  else()
    unset(CMAKE_REQUIRED_LIBRARIES)
  endif()

  if(OPENTELEMETRY_CPP_CONTRIB_SYSTEM_LIBRARIES)
    target_link_libraries(opentelemetry_prometheus_push_exporter
                          PUBLIC ${OPENTELEMETRY_CPP_CONTRIB_SYSTEM_LIBRARIES})
  endif()
endif()

install(
  TARGETS opentelemetry_prometheus_push_exporter
  EXPORT "${PROJECT_NAME}-target"
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY include/opentelemetry/exporters/prometheus
  DESTINATION include/opentelemetry/exporters/
  FILES_MATCHING
  PATTERN "*.h")

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

include(CMakePackageConfigHelpers)
file(MAKE_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}")

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}-config.cmake.in"
  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  NO_CHECK_REQUIRED_COMPONENTS_MACRO)

write_basic_package_version_file(
  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config-version.cmake"
  VERSION "${PROJECT_VERSION}${OPENTELEMETRY_CPP_ECOSYSTEM_VERSION_TWEAK}"
  COMPATIBILITY SameMajorVersion)

install(
  FILES
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake"
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config-version.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

export(
  EXPORT "${PROJECT_NAME}-target"
  NAMESPACE ${PROJECT_NAME}::
  FILE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-target.cmake"
)
install(
  EXPORT "${PROJECT_NAME}-target"
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
