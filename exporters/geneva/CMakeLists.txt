cmake_minimum_required(VERSION 3.12)

project(opentelemetry-fluentd)

add_definitions(-DHAVE_CONSOLE_LOG)

find_package(opentelemetry-cpp REQUIRED)
find_package(CURL)
include_directories(include)
# create fluentd trace exporter
add_library(opentelemetry_exporter_geneva_metrics
            src/exporter.cc src/unix_domain_socket_data_transport.cc)
target_include_directories(opentelemetry_exporter_geneva_metrics
                           PRIVATE ${OPENTELEMETRY_CPP_INCLUDE_DIRS})
target_link_libraries(opentelemetry_exporter_geneva_metrics
                      PUBLIC ${OPENTELEMETRY_CPP_LIBRARIES})
set_target_properties(opentelemetry_exporter_geneva_metrics
                      PROPERTIES EXPORT_NAME metrics)

if(BUILD_TESTING)
  include(GoogleTest)
  # build trace exporter tests
  add_executable(
    geneva_metrics_exporter_test test/trace/metrics_exporter_test.cc
                test/kaitai-struct/ifx_metrics_bin.cpp)

  target_link_libraries(
    geneva_metrics_exporter_test
    gtest
    gtest_main
    ${CMAKE_THREAD_LIBS_INIT}
    opentelemetry_exporter_geneva_metrics)

  gtest_add_tests(
    TARGET geneva_metrics_exporter_test
    TEST_PREFIX exporter.
    TEST_LIST geneva_metrics_exporter_test)
endif()