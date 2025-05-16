if(WIN32)
  message(FATAL_ERROR "user_events exporter is Linux only for now")
endif()

function (add_LinuxTracepoints)
  FetchContent_Declare(
    LinuxTracepoints
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/LinuxTracepoints
  )
  set(BUILD_SAMPLES OFF)
  set(BUILD_TOOLS OFF)
  FetchContent_MakeAvailable(LinuxTracepoints)
endfunction()

if(BUILD_TRACEPOINTS)
  add_LinuxTracepoints()
endif()

add_library(opentelemetry_exporter_user_events_logs
            src/logs_exporter.cc src/recordable.cc src/utils.cc)

set_target_properties(opentelemetry_exporter_user_events_logs
                      PROPERTIES EXPORT_NAME user_events_logs_exporter)

target_compile_features(opentelemetry_exporter_user_events_logs
                        PRIVATE cxx_std_17)

target_compile_definitions(opentelemetry_exporter_user_events_logs
                           PUBLIC HAVE_CONSOLE_LOG HAVE_LOGS_PREVIEW)

target_include_directories(opentelemetry_exporter_user_events_logs
                           PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:include>)

target_link_libraries(
    opentelemetry_exporter_user_events_logs
    PUBLIC 
    opentelemetry-cpp::logs 
    opentelemetry-cpp::version
    eventheader-tracepoint 
    tracepoint
    )

add_library(opentelemetry_exporter_user_events_metrics src/metrics_exporter.cc)

target_compile_features(opentelemetry_exporter_user_events_metrics
                        PRIVATE cxx_std_17)

set_target_properties(opentelemetry_exporter_user_events_metrics
                  PROPERTIES EXPORT_NAME user_events_metrics_exporter)

target_include_directories(opentelemetry_exporter_user_events_metrics
                  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                  $<INSTALL_INTERFACE:include>)

target_link_libraries(
    opentelemetry_exporter_user_events_metrics
    PUBLIC 
    opentelemetry-cpp::metrics 
    opentelemetry-cpp::otlp_recordable 
    tracepoint)

if(WITH_EXAMPLES)
  add_executable(user_events_logs example/logs/main.cc
                                  example/logs/foo_library.cc)
  target_link_libraries(user_events_logs PRIVATE opentelemetry_exporter_user_events_logs)

  add_executable(user_events_metrics example/metrics/main.cc
                                     example/metrics/foo_library.cc)
  target_link_libraries(user_events_metrics PRIVATE opentelemetry_exporter_user_events_metrics)
endif()

if(BUILD_TESTING)
  add_executable(user_events_logs_exporter_test test/logs_exporter_test.cc)
  target_link_libraries(
    user_events_logs_exporter_test 
    PRIVATE 
    opentelemetry_exporter_user_events_logs
    Threads::Threads
    GTest::gtest GTest::gtest_main)

  gtest_add_tests(
    TARGET user_events_logs_exporter_test
    TEST_PREFIX user_events.
    TEST_LIST user_events_logs_exporter_test)
endif()

if(WITH_BENCHMARK)
  add_executable(user_events_logger_benchmark benchmark/logger_benchmark.cc)
  target_link_libraries(
    user_events_logger_benchmark PRIVATE benchmark::benchmark opentelemetry-cpp::logs
    opentelemetry_exporter_user_events_logs)
endif()
 
otel_add_component(
  COMPONENT exporters_user_events
  TARGETS opentelemetry_exporter_user_events_logs opentelemetry_exporter_user_events_metrics
  FILES_DIRECTORY include/opentelemetry/exporters/user_events
  FILES_DESTINATION include/opentelemetry/exporters
  FILES_MATCHING PATTERN "*.h")
    