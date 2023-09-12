if("${opentelemetry-cpp-tag}" STREQUAL "")
	set(opentelemetry-cpp-tag "v1.9.1")
endif()
function(target_create _target _lib)
  add_library(${_target} STATIC IMPORTED)
  set_target_properties(
    ${_target} PROPERTIES IMPORTED_LOCATION
                          "${opentelemetry_BINARY_DIR}/${_lib}")
endfunction()

function(build_opentelemetry)
  set(opentelemetry_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/opentelemetry-cpp")
  set(opentelemetry_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/opentelemetry-cpp")
  set(opentelemetry_cpp_targets opentelemetry_trace opentelemetry_logs)
  set(opentelemetry_CMAKE_ARGS -DCMAKE_POSITION_INDEPENDENT_CODE=ON
          -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
          -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
          -DWITH_LOGS_PREVIEW=ON
          -DBUILD_TESTING=OFF
          -DWITH_EXAMPLES=OFF)

  set(opentelemetry_libs
    ${opentelemetry_BINARY_DIR}/sdk/src/trace/libopentelemetry_trace.a
    ${opentelemetry_BINARY_DIR}/sdk/src/logs/libopentelemetry_logs.a
    ${opentelemetry_BINARY_DIR}/sdk/src/resource/libopentelemetry_resources.a
    ${opentelemetry_BINARY_DIR}/sdk/src/common/libopentelemetry_common.a
    ${CURL_LIBRARIES}
  )

  set(opentelemetry_include_dir ${opentelemetry_SOURCE_DIR}/api/include/
     ${opentelemetry_SOURCE_DIR}/ext/include/
     ${opentelemetry_SOURCE_DIR}/sdk/include/
  )

  include_directories(SYSTEM ${opentelemetry_include_dir})

  set(opentelemetry_deps opentelemetry_trace opentelemetry_logs opentelemetry_resources opentelemetry_common ${CURL_LIBRARIES})

   set(make_cmd ${CMAKE_COMMAND} --build <BINARY_DIR> --target
     ${opentelemetry_cpp_targets})

   include(ExternalProject)
   ExternalProject_Add(
    opentelemetry-cpp
    GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-cpp.git
    GIT_TAG  "${opentelemetry-cpp-tag}"
    GIT_SUBMODULES "third_party/opentelemetry-proto"
    SOURCE_DIR ${opentelemetry_SOURCE_DIR}
    PREFIX "opentelemetry-cpp"
    CMAKE_ARGS ${opentelemetry_CMAKE_ARGS}
    BUILD_COMMAND ${make_cmd}
    BINARY_DIR ${opentelemetry_BINARY_DIR}
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${opentelemetry_libs}
    DEPENDS ${dependencies}
    LOG_BUILD ON)

    target_create("opentelemetry_trace" "sdk/src/trace/libopentelemetry_trace.a")
    target_create("opentelemetry_logs" "sdk/src/logs/libopentelemetry_logs.a")
    target_create("opentelemetry_resources"
                  "sdk/src/resource/libopentelemetry_resources.a")
    target_create("opentelemetry_common"
                  "sdk/src/common/libopentelemetry_common.a")
    add_library(opentelemetry::libopentelemetry INTERFACE IMPORTED)
    add_dependencies(opentelemetry::libopentelemetry opentelemetry-cpp)
    set_target_properties(
        opentelemetry::libopentelemetry
        PROPERTIES
        INTERFACE_LINK_LIBRARIES "${opentelemetry_deps}")
endfunction()
