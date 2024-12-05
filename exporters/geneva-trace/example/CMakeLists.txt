add_executable(opentelemetry-geneva-trace main.cc)

target_link_libraries(opentelemetry-geneva-trace  ${CMAKE_THREAD_LIBS_INIT}
                      opentelemetry_api opentelemetry_exporter_etw)