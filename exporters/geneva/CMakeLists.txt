set(OTEL_GENEVA_EXPORTER_VERSION 1.0.0)
set(OTEL_GENEVA_EXPORTER_MAJOR_VERSION 1)

function(set_target_version target_name)
  if(OTELCPP_VERSIONED_LIBS)
    set_target_properties(
      ${target_name} PROPERTIES VERSION ${OTEL_GENEVA_EXPORTER_VERSION}
                                SOVERSION ${OTEL_GENEVA_EXPORTER_MAJOR_VERSION})
  endif()
endfunction()

add_library(opentelemetry_exporter_geneva_metrics
              src/exporter.cc 
              src/unix_domain_socket_data_transport.cc)

if(WIN32)
  target_sources(
    opentelemetry_exporter_geneva_metrics
    src/etw_data_transport.cc)
endif()

set_target_version(opentelemetry_exporter_geneva_metrics)

target_compile_definitions(opentelemetry_exporter_geneva_metrics PRIVATE HAVE_CONSOLE_LOG)

set_target_properties(opentelemetry_exporter_geneva_metrics PROPERTIES EXPORT_NAME geneva_metrics_exporter)

target_include_directories(opentelemetry_exporter_geneva_metrics
                       PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                       $<INSTALL_INTERFACE:include>)

target_link_libraries(opentelemetry_exporter_geneva_metrics
                      PUBLIC opentelemetry-cpp::metrics)

if(BUILD_TESTING)
  # build geneva metrics tests
  add_executable(
    geneva_metrics_exporter_test
    test/metrics_exporter_test.cc 
    test/decoder/ifx_metrics_bin.cpp
    test/decoder/kaitai/kaitaistream.cpp)

  target_include_directories(geneva_metrics_exporter_test PRIVATE 
    test/common
    test/decoder)

  target_link_libraries(
    geneva_metrics_exporter_test PRIVATE  
    opentelemetry_exporter_geneva_metrics
    GTest::gtest GTest::gtest_main
    Threads::Threads)

  target_compile_definitions(geneva_metrics_exporter_test PRIVATE KS_STR_ENCODING_NONE)
  
  gtest_add_tests(
    TARGET geneva_metrics_exporter_test
    TEST_PREFIX geneva.
    TEST_LIST geneva_metrics_exporter_test)
endif()

if(WITH_EXAMPLES)
  add_executable(example_metrics example/example_metrics.cc
                                 example/foo_library.cc)
  target_link_libraries(example_metrics opentelemetry_exporter_geneva_metrics)

  if(NOT WIN32)
    add_executable(stress_test_linux example/stress_test_linux.cc)
    target_link_libraries(stress_test_linux opentelemetry_exporter_geneva_metrics)

    find_package(Boost CONFIG QUIET COMPONENTS system uuid)
    if(Boost_FOUND)
      target_compile_definitions(stress_test_linux PRIVATE HAVE_BOOST)
      target_link_libraries(stress_test_linux PRIVATE Boost::uuid)
    endif()
  endif()
endif()

otel_add_component(
  COMPONENT exporters_geneva_metrics
  TARGETS opentelemetry_exporter_geneva_metrics
  FILES_DIRECTORY include/opentelemetry/exporters/geneva
  FILES_DESTINATION include/opentelemetry/exporters
  FILES_MATCHING PATTERN "*.h")