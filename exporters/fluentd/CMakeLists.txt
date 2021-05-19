# Copyright 2021, OpenTelemetry Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include_directories(include)

add_library(opentelemetry_exporter_fluentd_trace src/fluentd_exporter.cc
                                                 src/recordable.cc)

target_link_libraries(opentelemetry_exporter_fluentd_trace
                      PUBLIC opentelemetry_common)

install(
  TARGETS opentelemetry_exporter_fluentd_trace
  EXPORT "${PROJECT_NAME}-target"
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY include/opentelemetry/exporters/fluentd
  DESTINATION include/opentelemetry/exporters/
  FILES_MATCHING
  PATTERN "*.h")

if(BUILD_TESTING)
  add_executable(fluentd_recordable_test test/fluentd_recordable_test.cc test/fluentd_baseline_test.cc)

  target_link_libraries(
    fluentd_recordable_test
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    opentelemetry_common
    opentelemetry_trace
    opentelemetry_resources
    opentelemetry_sdk
    opentelemetry_exporter_fluentd_trace)

  gtest_add_tests(
    TARGET fluentd_recordable_test
    TEST_PREFIX exporter.
    TEST_LIST fluentd_recordable_test)
endif() # BUILD_TESTING
