// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include <string>
#pragma once

#ifndef strncpy_s
#define strncpy_s(dest, destsz, src, count)                                    \
  strncpy(dest, src, (destsz <= count) ? destsz : count)
#endif

#ifndef NO_OTEL_STATSD_DEBUG
#define DEBUG_MSG(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG(...)
#endif