# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

if(WITH_COMPONENT_GENEVA)
  message(STATUS "Building with Geneva exporters")
  add_subdirectory(geneva)
endif() 

if(WITH_COMPONENT_USER_EVENTS)
  message(STATUS "Building with User Events exporters")
  add_subdirectory(user_events)
endif()