if(WITH_BENCHMARK)
  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG 192ef10025eb2c4cdd392bc502f0c852196baa48 # v1.9.5
  )
  FetchContent_MakeAvailable(benchmark)
  add_executable(user_events_logger_benchmark logger_benchmark.cc)
  target_link_libraries(
    user_events_logger_benchmark benchmark::benchmark
    ${CMAKE_THREAD_LIBS_INIT} opentelemetry_logs
    opentelemetry_exporter_user_events_logs)
endif()
