foreach(testname push_exporter_test)
  add_executable(${testname} "${testname}.cc")
  target_link_libraries(
    ${testname}
    GTest::gtest
    GTest::gtest_main
    opentelemetry_prometheus_push_exporter
    prometheus-cpp::push
    ${CMAKE_THREAD_LIBS_INIT}
    ${ATFRAMEWORK_CMAKE_TOOLSET_SYSTEM_LIBRARIES})
  gtest_discover_tests(
    ${testname}
    TEST_PREFIX opentelemetry_cpp_ecosystem_test.exporter.prometheus.
    TEST_LIST prometheus_exporter_test)
endforeach()
