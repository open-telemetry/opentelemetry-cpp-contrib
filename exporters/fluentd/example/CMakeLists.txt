add_executable(fluentd_example_trace trace/main.cc)

target_link_libraries(fluentd_example_trace ${CMAKE_THREAD_LIBS_INIT} trace/foo_library/foo_library.cc
                      opentelemetry_exporter_fluentd_trace opentelemetry-cpp::trace)

add_executable(fluentd_example_logs log/main.cc)

target_link_libraries(fluentd_example_logs ${CMAKE_THREAD_LIBS_INIT} trace/foo_library/foo_library.cc
                      opentelemetry_exporter_fluentd_logs opentelemetry-cpp::logs)
