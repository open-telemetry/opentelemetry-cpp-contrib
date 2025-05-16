find_package(benchmark CONFIG QUIET)
if(NOT benchmark_FOUND)
  message(STATUS "benchmark not found, fetching...")
  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY
    https://github.com/google/benchmark.git
    GIT_TAG
    299e5928955cc62af9968370293b916f5130916f #v1.9.3
  )
  FetchContent_MakeAvailable(benchmark)
endif()

if(NOT benchmark_VERSION)
  message(FATAL_ERROR "benchmark version not found")
endif()

if(NOT TARGET benchmark::benchmark)
  message(FATAL_ERROR "benchmark not found")
endif()