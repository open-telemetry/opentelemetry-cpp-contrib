# Fetch opentelemetry-cpp: 
#   This is required for the cmake install functions. 
FetchContent_Declare(
  opentelemetry-cpp
  GIT_REPOSITORY
  https://github.com/dbarker/opentelemetry-cpp.git
  GIT_TAG
  poc_otel_cmake_external_repo_support
)

# Alternatively fetch from an opentelemetry-cpp local src directory
# FetchContent_Declare(
#  opentelemetry-cpp
#  SOURCE_DIR "/workspaces/opentelemetry-cpp"
#  )

if(opentelemetry-cpp_PROVIDER STREQUAL "package")
  find_package(opentelemetry-cpp CONFIG REQUIRED)
  # silence CMP0169 deprecation of one-arg Populate()
  cmake_policy(SET CMP0169 OLD)
  # Populate the opentelemetry-cpp src to enable using the cmake install functions
  FetchContent_Populate(opentelemetry-cpp)
elseif(opentelemetry-cpp_PROVIDER STREQUAL "fetch")
  set(OPENTELEMETRY_INSTALL ${OPENTELEMETRY_CONTRIB_INSTALL})
  # If user events is enabled, set the otlp file to ON to make the otlp_recordable target available
  if(WITH_COMPONENT_USER_EVENTS)
    set(WITH_OTLP_FILE ON)
  endif()
  # Add opetelemetry-cpp src to the build tree
  FetchContent_MakeAvailable(opentelemetry-cpp)
  get_target_property(opentelemetry-cpp_VERSION opentelemetry-cpp::api INTERFACE_OPENTELEMETRY_VERSION)
  get_target_property(OPENTELEMETRY_ABI_VERSION_NO opentelemetry-cpp::api INTERFACE_OPENTELEMETRY_ABI_VERSION_NO)
endif()

message(STATUS "opentelemetry-cpp PROVIDER: ${opentelemetry-cpp_PROVIDER}")
message(STATUS "opentelemetry-cpp SOURCE_DIR: ${opentelemetry-cpp_SOURCE_DIR}")
message(STATUS "opentelemetry-cpp VERSION: ${opentelemetry-cpp_VERSION}")
message(STATUS "opentelemetry-cpp ABI_VERSION_NO: ${OPENTELEMETRY_ABI_VERSION_NO}")
