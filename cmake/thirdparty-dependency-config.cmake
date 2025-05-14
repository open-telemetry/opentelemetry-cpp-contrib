# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

#-----------------------------------------------------------------------
# Third party dependencies supported by opentelemetry-cpp-contrib
# Dependencies that must be found with find_dependency() when a user calls find_package(opentelemetry-cpp-contrib ...)
# should be included in this list.
#-----------------------------------------------------------------------
set(OTEL_THIRDPARTY_DEPENDENCIES_SUPPORTED
     opentelemetry-cpp
     tracepoint
)

#-----------------------------------------------------------------------
# Third party dependency target namespaces. Defaults to the dependency's project name if not set.
# Only set if the target namespace is different from the project name (these are case sensitive).
# set(OTEL_<dependency>_TARGET_NAMESPACE "<namespace>")
#-----------------------------------------------------------------------
# set(OTEL_Protobuf_TARGET_NAMESPACE "protobuf")

#-----------------------------------------------------------------------
# Set the find_dependecy search mode - empty is default. Options: cmake default (empty string ""), "MODULE", or "CONFIG"
# # set(OTEL_<dependency>_SEARCH_MODE "<search mode>")
#-----------------------------------------------------------------------
set(OTEL_opentelemetry-cpp_SEARCH_MODE "CONFIG")
set(OTEL_tracepoint_SEARCH_MODE "CONFIG")
