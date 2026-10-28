cmake_minimum_required(VERSION 3.12)

project(opentelemetry-process-metrics)

add_definitions(-DHAVE_CONSOLE_LOG)

find_package(opentelemetry-cpp REQUIRED)
find_package(CURL REQUIRED)

option(WITH_EXAMPLE "Whether to build example code" ON)


include_directories(include)
if(UNIX AND NOT APPLE)
    message("LINUX...")
    add_library(opentelemetry_process_metrics src/linux/process_cpu_time.cc src/linux/process_metrics_linux_factory.cc  src/test_lib.cc)
else()
    message("SOMETHIG ELSE...")
    add_library(opentelemetry_process_metrics src/noop/process_metrics_noop_factory.cc src/test_lib.cc)
endif()

if (WITH_EXAMPLE)
    add_executable(metrics_example example/metrics_example.cc)
    target_link_libraries(metrics_example opentelemetry_process_metrics opentelemetry-cpp::metrics
    opentelemetry-cpp::resources opentelemetry_exporter_ostream_metrics)
endif()
