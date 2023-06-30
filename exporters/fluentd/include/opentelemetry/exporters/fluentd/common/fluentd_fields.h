// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0
#pragma once

/* clang-format off */

/**
 * List of configurable Field Name constants:
 *
 * - env_ver                      - Schema version (optional)
 * - env_name
 * - env_time
 * - env_dt_spanId                - OT SpanId
 * - env_dt_traceId               - OT TraceId
 * - startTime                    - OT Span start time
 * - kind                         - OT Span kind
 * - name                         - OT Span name
 * - parentId                     - OT Span parentId
 * - links                        - OT Span links array
 */

#  define FLUENT_FIELD_VERSION              "env_ver"               /* Event version           */
#  define FLUENT_FIELD_TYPE                 "env_type"              /* Event type              */
#  define FLUENT_FIELD_NAME                 "name"                  /* Event name              */

#ifndef HAVE_FIELD_TIME
#define HAVE_FIELD_TIME
#endif
#  define FLUENT_FIELD_TIME                 "env_time"              /* Event time at envelope  */

#  define FLUENT_FIELD_TRACE_ID             "env_dt_traceId"        /* Trace Id                */
#  define FLUENT_FIELD_SPAN_ID              "env_dt_spanId"         /* Span Id                 */
#  define FLUENT_FIELD_SPAN_PARENTID        "parentId"              /* Span ParentId           */
#  define FLUENT_FIELD_SPAN_KIND            "kind"                  /* Span Kind               */
#  define FLUENT_FIELD_SPAN_LINKS           "links"                 /* Span Links array        */

#  define FLUENT_FIELD_PROPERTIES           "env_properties"

/* Span option constants */
#  define FLUENT_FIELD_STARTTIME            "startTime"             /* Operation start time    */
#  define FLUENT_FIELD_ENDTTIME             "env_time"              /* Operation end time      */
#  define FLUENT_FIELD_DURATION             "duration"              /* Operation duration      */
#  define FLUENT_FIELD_STATUSCODE           "statusCode"            /* OT Span status code     */
#  define FLUENT_FIELD_STATUSMESSAGE        "statusMessage"         /* OT Span status message  */
#  define FLUENT_FIELD_SUCCESS              "success"               /* OT Span success         */

/*Log option constants */
# define FLUENT_FIELD_TIMESTAMP             "Timestamp"              /* Log timestamp  */
# define FLUENT_FIELD_OBSERVEDTIMESTAMP     "ObservedTimestamp"              /* Log observed timestamp  */

/* Value constants */
#  define FLUENT_VALUE_SPAN                 "Span"                  /* Event name for Span     */
#  define FLUENT_VALUE_LOG                  "Log"                  /* Event name for Log     */

#  define FLUENT_VALUE_SPAN_START           "SpanStart"             /* Span Start              */
#  define FLUENT_VALUE_SPAN_END             "SpanEnd"               /* Span Start              */

/* clang-format on */
