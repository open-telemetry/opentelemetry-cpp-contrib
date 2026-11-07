set(OTEL_DEPS_DIR "" CACHE PATH "Prebuilt dependency tree")

if(NOT OTEL_DEPS_DIR)
    message(FATAL_ERROR
        "OTEL_DEPS_DIR is not set. Point it at the prebuilt dependency tree\n")
endif()

if(NOT IS_DIRECTORY "${OTEL_DEPS_DIR}")
    message(FATAL_ERROR "OTEL_DEPS_DIR does not exist: ${OTEL_DEPS_DIR}")
endif()

set(OTEL_APR_VERSION      "1.7.0"  CACHE STRING "apr version in the deps tree")
set(OTEL_APRUTIL_VERSION  "1.6.1"  CACHE STRING "apr-util version in the deps tree")
set(OTEL_EXPAT_VERSION    "2.3.0"  CACHE STRING "expat version in the deps tree")
set(OTEL_LOG4CXX_VERSION  "0.11.0" CACHE STRING "log4cxx version in the deps tree")
set(OTEL_CPP_SDK_VERSION  "1.2.0"  CACHE STRING "opentelemetry-cpp version in the deps tree")

set(OTEL_APR_ROOT      "${OTEL_DEPS_DIR}/apr/${OTEL_APR_VERSION}")
set(OTEL_APRUTIL_ROOT  "${OTEL_DEPS_DIR}/apr-util/${OTEL_APRUTIL_VERSION}")
set(OTEL_EXPAT_ROOT    "${OTEL_DEPS_DIR}/expat/${OTEL_EXPAT_VERSION}")
set(OTEL_LOG4CXX_ROOT  "${OTEL_DEPS_DIR}/apache-log4cxx/${OTEL_LOG4CXX_VERSION}")
set(OTEL_CPP_SDK_ROOT  "${OTEL_DEPS_DIR}/opentelemetry/${OTEL_CPP_SDK_VERSION}")

# Declares an IMPORTED library target for one prebuilt dependency.
#
#   otel_declare_prebuilt(<target> <kind> <root> <libname>)
#
#   target  - name to create, e.g. otel::deps::apr
#   kind    - STATIC or SHARED
#   root    - install prefix, e.g. ${OTEL_APR_ROOT}
#   libname - base name passed to find_library, e.g. apr-1
function(otel_declare_prebuilt target kind root libname)
    if(NOT kind MATCHES "^(STATIC|SHARED)$")
        message(FATAL_ERROR "otel_declare_prebuilt(${target}): kind must be STATIC or SHARED, got '${kind}'")
    endif()

    string(MAKE_C_IDENTIFIER "OTEL_PREBUILT_${target}" _cache_var)

    find_library(${_cache_var}
        NAMES ${libname}
        PATHS "${root}"
        PATH_SUFFIXES lib lib64
        NO_DEFAULT_PATH)

    if(NOT ${_cache_var})
        message(FATAL_ERROR
            "${target}: no lib${libname} under ${root}/{lib,lib64}.\n"
            "Check OTEL_DEPS_DIR and the OTEL_*_VERSION cache variables.")
    endif()

    add_library(${target} ${kind} IMPORTED GLOBAL)
    set_target_properties(${target} PROPERTIES IMPORTED_LOCATION "${${_cache_var}}")

    if(IS_DIRECTORY "${root}/include")
        set_target_properties(${target} PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${root}/include")
    endif()
endfunction()

otel_declare_prebuilt(otel::deps::apr     STATIC "${OTEL_APR_ROOT}"     apr-1)
otel_declare_prebuilt(otel::deps::aprutil STATIC "${OTEL_APRUTIL_ROOT}" aprutil-1)
otel_declare_prebuilt(otel::deps::expat   STATIC "${OTEL_EXPAT_ROOT}"   expat)
otel_declare_prebuilt(otel::deps::log4cxx STATIC "${OTEL_LOG4CXX_ROOT}" log4cxx)

set(OTEL_CPP_SDK_LIBS
    opentelemetry_common
    opentelemetry_resources
    opentelemetry_trace
    opentelemetry_otlp_recordable
    opentelemetry_exporter_ostream_span
    opentelemetry_exporter_otlp_grpc)

foreach(_lib IN LISTS OTEL_CPP_SDK_LIBS)
    otel_declare_prebuilt(otel::deps::${_lib} SHARED "${OTEL_CPP_SDK_ROOT}" ${_lib})
endforeach()

# Must match BOOST_VERSION in the docker/*/Dockerfile images and codeql-env.sh.
set(OTEL_BOOST_VERSION "1.75.0" CACHE STRING "boost version in the deps tree")
set(OTEL_BOOST_ROOT "${OTEL_DEPS_DIR}/boost/${OTEL_BOOST_VERSION}")

otel_declare_prebuilt(otel::deps::boost_filesystem STATIC "${OTEL_BOOST_ROOT}" boost_filesystem)
otel_declare_prebuilt(otel::deps::boost_system     STATIC "${OTEL_BOOST_ROOT}" boost_system)
