include_guard(DIRECTORY)

function(otelcpp_contrib_tools_set_export_declaration OUTPUT_VARNAME)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang|Intel|XL|XLClang")
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
      set(${OUTPUT_VARNAME}
          "__attribute__((__dllexport__))"
          PARENT_SCOPE)
    else()
      set(${OUTPUT_VARNAME}
          "__attribute__((visibility(\"default\")))"
          PARENT_SCOPE)
    endif()
  elseif(MSVC)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
      set(${OUTPUT_VARNAME}
          "__declspec(dllexport)"
          PARENT_SCOPE)
    else()
      set(${OUTPUT_VARNAME}
          ""
          PARENT_SCOPE)
    endif()
  elseif(SunPro)
    set(${OUTPUT_VARNAME}
        "__global"
        PARENT_SCOPE)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(${OUTPUT_VARNAME}
        "__declspec(dllexport)"
        PARENT_SCOPE)
  else()
    set(${OUTPUT_VARNAME}
        ""
        PARENT_SCOPE)
  endif()
endfunction()

function(otelcpp_contrib_tools_set_import_declaration OUTPUT_VARNAME)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang|Intel|XL|XLClang")
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
      set(${OUTPUT_VARNAME}
          "__attribute__((__dllimport__))"
          PARENT_SCOPE)
    else()
      set(${OUTPUT_VARNAME}
          ""
          PARENT_SCOPE)
    endif()
  elseif(MSVC)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
      set(${OUTPUT_VARNAME}
          "__declspec(dllimport)"
          PARENT_SCOPE)
    else()
      set(${OUTPUT_VARNAME}
          ""
          PARENT_SCOPE)
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "SunPro")
    set(${OUTPUT_VARNAME}
        "__global"
        PARENT_SCOPE)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(${OUTPUT_VARNAME}
        "__declspec(dllimport)"
        PARENT_SCOPE)
  else()
    set(${OUTPUT_VARNAME}
        ""
        PARENT_SCOPE)
  endif()
endfunction()

function(otelcpp_contrib_tools_set_shared_library_declaration
         DEFINITION_VARNAME)
  otelcpp_contrib_tools_set_export_declaration(EXPORT_DECLARATION)
  otelcpp_contrib_tools_set_import_declaration(IMPORT_DECLARATION)
  foreach(TARGET_NAME ${ARGN})
    target_compile_definitions(
      ${TARGET_NAME} INTERFACE "${DEFINITION_VARNAME}=${IMPORT_DECLARATION}")
    target_compile_definitions(
      ${TARGET_NAME} PRIVATE "${DEFINITION_VARNAME}=${EXPORT_DECLARATION}")
  endforeach()
endfunction()

function(otelcpp_contrib_tools_set_static_library_declaration
         DEFINITION_VARNAME)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang|Intel|XL|XLClang")
    foreach(TARGET_NAME ${ARGN})
      target_compile_definitions(
        ${TARGET_NAME}
        PUBLIC "${DEFINITION_VARNAME}=__attribute__((visibility(\"default\")))")
    endforeach()
  else()
    foreach(TARGET_NAME ${ARGN})
      target_compile_definitions(${TARGET_NAME} PUBLIC "${DEFINITION_VARNAME}=")
    endforeach()
  endif()
endfunction()
