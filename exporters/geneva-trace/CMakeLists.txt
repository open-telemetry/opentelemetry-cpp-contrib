cmake_minimum_required(VERSION 3.12)

# MAIN_PROJECT CHECK
# determine if this project is built as a subproject (using add_subdirectory) or if this is the main project
set(MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  project(opentelemetry-geneva-trace-log-exporter)
  set(MAIN_PROJECT ON)
endif()

if(MAIN_PROJECT)
  option(WITH_EXAMPLES "Build examples" ON)
endif()

if(WITH_EXAMPLES)
  include_directories(include)
  add_subdirectory(example)
endif()

if(OPENTELEMETRY_INSTALL)
  install(DIRECTORY include/ DESTINATION include)
endif()