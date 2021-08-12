find_package(opentelemetry-cpp REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/exporters/ostream/include)

add_library(foo_library foo_library/foo_library.cc)

target_link_libraries(foo_library opentelemetry_exporter_fluentd_trace
                      ${CMAKE_THREAD_LIBS_INIT})

add_executable(fluentd_example main.cc)
target_link_libraries(fluentd_example ${CMAKE_THREAD_LIBS_INIT} foo_library
                      opentelemetry_trace)
