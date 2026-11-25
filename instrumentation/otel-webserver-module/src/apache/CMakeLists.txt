set(OTEL_APACHE22_INCLUDE_DIR "" CACHE PATH "httpd 2.2 include dir")
set(OTEL_APACHE24_INCLUDE_DIR "" CACHE PATH "httpd 2.4 include dir")

set(_apache_sources
    ApacheTracing.cpp
    ApacheConfig.cpp
    ExcludedModules.cpp
    ApacheHooks.cpp
    HookContainer.cpp
    mod_apache_otel.cpp)

function(otel_add_apache_module target httpd_include_dir)
    if(NOT httpd_include_dir)
        message(FATAL_ERROR "${target}: httpd include dir is empty; set OTEL_APACHE22_INCLUDE_DIR / OTEL_APACHE24_INCLUDE_DIR")
    endif()

    add_library(${target} SHARED ${_apache_sources})

    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include/core"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include/util"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include/apache"
        "${httpd_include_dir}"
        "${httpd_include_dir}/../os/unix"
        "${OTEL_APR_ROOT}/include/apr-1"
        "${OTEL_APRUTIL_ROOT}/include/apr-1")

    target_compile_options(${target} PRIVATE
        -pipe -Wall -fexceptions -fstack-protector --param=ssp-buffer-size=4
        -mtune=generic -fno-strict-aliasing -Wno-unused-local-typedefs -O1
        ${OTEL_ARCH_FLAG})
    target_compile_definitions(${target} PRIVATE _FORTIFY_SOURCE=1)

    set_target_properties(${target} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/apache")

    target_link_libraries(${target} PRIVATE opentelemetry_webserver_sdk)

    target_link_options(${target} PRIVATE
        "LINKER:--no-as-needed"
        "LINKER:--exclude-libs=ALL")
endfunction()

otel_add_apache_module(mod_apache_otel   "${OTEL_APACHE24_INCLUDE_DIR}")
otel_add_apache_module(mod_apache_otel22 "${OTEL_APACHE22_INCLUDE_DIR}")
