# Must match GTEST_VERSION in docker/ubuntu22.04/Dockerfile and
# docker/almalinux8/Dockerfile.
set(OTEL_GTEST_VERSION "1.12.1" CACHE STRING "googletest version in the deps tree")
set(OTEL_GTEST_ROOT "${OTEL_DEPS_DIR}/googletest/${OTEL_GTEST_VERSION}")

foreach(_g gmock gmock_main gtest gtest_main)
    otel_declare_prebuilt(otel::deps::${_g} STATIC "${OTEL_GTEST_ROOT}" ${_g})
endforeach()

add_executable(unit_test
    unit_test.cpp
    ApiUtils_test.cpp
    ScopedSpan_test.cpp
    ServerSpan_test.cpp
    SdkWrapper_test.cpp
    SdkUtils_test.cpp
    WSAgent_test.cpp
    AgentCore_test.cpp
    RequestProcessingEngine_test.cpp
    SpanNamer_test.cpp
    integration_test.cpp)

target_compile_definitions(unit_test PRIVATE HAVE_ABSEIL)

target_include_directories(unit_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include/util"
    "${OTEL_GTEST_ROOT}/include"
    "${OTEL_CPP_SDK_ROOT}/include"
    "${OTEL_LOG4CXX_ROOT}/include"
    "${OTEL_APR_ROOT}/include"
    "${OTEL_APRUTIL_ROOT}/include")

target_compile_options(unit_test PRIVATE -Wall -pthread)

target_link_libraries(unit_test PRIVATE
    opentelemetry_webserver_sdk
    otel::deps::gmock_main
    otel::deps::gmock
    otel::deps::gtest_main
    otel::deps::gtest
    -Wl,--start-group
        otel::deps::log4cxx
        otel::deps::aprutil
        otel::deps::apr
        otel::deps::expat
    -Wl,--end-group
    otel::deps::opentelemetry_common
    otel::deps::opentelemetry_resources
    otel::deps::opentelemetry_trace
    otel::deps::opentelemetry_otlp_recordable
    otel::deps::opentelemetry_exporter_ostream_span
    otel::deps::opentelemetry_exporter_otlp_grpc
    otel::deps::boost_filesystem
    otel::deps::boost_system
    Threads::Threads
    rt dl z crypt)

target_link_options(unit_test PRIVATE "LINKER:--enable-new-dtags")

set_target_properties(unit_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test")

add_test(NAME unit_test COMMAND unit_test)
