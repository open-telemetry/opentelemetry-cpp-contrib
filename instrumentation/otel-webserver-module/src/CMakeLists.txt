add_library(opentelemetry_webserver_sdk SHARED
    core/api/WSAgent.cpp
    core/api/RequestProcessingEngine.cpp
    core/api/ApiUtils.cpp
    core/api/SpanNamer.cpp
    core/api/opentelemetry_ngx_api.cpp
    core/AgentLogger.cpp
    core/AgentCore.cpp
    core/sdkwrapper/SdkHelperFactory.cpp
    core/sdkwrapper/ScopedSpan.cpp
    core/sdkwrapper/ServerSpan.cpp
    core/sdkwrapper/SdkWrapper.cpp
    util/SpanNamingUtils.cpp
    util/RegexResolver.cpp)

target_compile_definitions(opentelemetry_webserver_sdk PRIVATE TIMER_USE_CGT HAVE_ABSEIL)

target_include_directories(opentelemetry_webserver_sdk PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/util"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/core"
    "${OTEL_LOG4CXX_ROOT}/include"
    "${OTEL_CPP_SDK_ROOT}/include")

target_compile_options(opentelemetry_webserver_sdk PRIVATE
    -pthread -g -O1 ${OTEL_ARCH_FLAG})
target_compile_definitions(opentelemetry_webserver_sdk PRIVATE _FORTIFY_SOURCE=1)

# --start-group/--end-group: log4cxx depends on apr-util, which depends on apr
# and expat.
target_link_libraries(opentelemetry_webserver_sdk PRIVATE
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

target_link_options(opentelemetry_webserver_sdk PRIVATE
    "LINKER:--exclude-libs=ALL"
    "LINKER:--gc-sections"
    "LINKER:-z,defs")

set_target_properties(opentelemetry_webserver_sdk PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
