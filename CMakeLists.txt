cmake_minimum_required(VERSION 3.16)
project(opentelemetry_cpp_contrib LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  opentelemetry-cpp
  GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-cpp.git
  GIT_TAG b9cf499ff5715433848b316059714b5c59af1f2c # v1.21.0
)

FetchContent_MakeAvailable(opentelemetry-cpp)

# List all contrib instrumentation directories
set(OTEL_INSTRUMENTATION
  boostlog
  glog
  spdlog
)

# Create an ON/OFF option for each instrumentation
foreach(instr ${OTEL_INSTRUMENTATION})
  string(TOUPPER ${instr} INSTR_UPPER)
  option(INCLUDE_OTEL_INSTRUMENTATION_${INSTR_UPPER} "Include instrumentation ${instr}" ON)
endforeach()

# Add only the enabled instrumentation
foreach(instr ${OTEL_INSTRUMENTATION})
  string(TOUPPER ${instr} INSTR_UPPER)
  if(INCLUDE_OTEL_INSTRUMENTATION_${INSTR_UPPER})
    add_subdirectory(instrumentation/${instr})
  endif()
endforeach()
