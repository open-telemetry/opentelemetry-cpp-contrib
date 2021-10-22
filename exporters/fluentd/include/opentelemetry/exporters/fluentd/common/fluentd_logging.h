// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#if defined(HAVE_CONSOLE_LOG) && !defined(LOG_DEBUG)
// Log to console if there's no standard log facility defined
#include <cstdio>
#ifndef LOG_DEBUG
#define LOG_DEBUG(fmt_, ...) printf(" " fmt_ "\n", ##__VA_ARGS__)
#define LOG_TRACE(fmt_, ...) printf(" " fmt_ "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt_, ...) printf(" " fmt_ "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt_, ...) printf(" " fmt_ "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt_, ...) printf(" " fmt_ "\n", ##__VA_ARGS__)
#endif
#endif

#ifndef LOG_DEBUG
// Don't log anything if there's no standard log facility defined
#define LOG_DEBUG(fmt_, ...)
#define LOG_TRACE(fmt_, ...)
#define LOG_INFO(fmt_, ...)
#define LOG_WARN(fmt_, ...)
#define LOG_ERROR(fmt_, ...)
#endif
