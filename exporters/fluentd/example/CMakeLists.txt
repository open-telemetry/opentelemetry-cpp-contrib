find_package(opentelemetry-cpp REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/exporters/fluentd/include)

add_library(foo_library_trace trace/foo_library/foo_library.cc)

target_link_libraries(foo_library_trace opentelemetry_exporter_fluentd_trace
                      ${CMAKE_THREAD_LIBS_INIT})

add_executable(fluentd_example_trace trace/main.cc)
target_link_libraries(fluentd_example_trace ${CMAKE_THREAD_LIBS_INIT} foo_library_trace
                      opentelemetry_trace)



add_library(foo_library_logs log/foo_library/foo_library.cc)

target_link_libraries(foo_library_logs opentelemetry_exporter_fluentd_logs
                      ${CMAKE_THREAD_LIBS_INIT})

add_executable(fluentd_example_logs log/main.cc)
target_link_libraries(fluentd_example_logs ${CMAKE_THREAD_LIBS_INIT} foo_library_logs
                      opentelemetry_logs)
