// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

/* clang-format off */

/**
 * List of configurable Field Name constants:
 *
 * - env_ver                      - Schema version (optional for ETW exporter).
 * - env_name                     - Built-in ETW name at envelope level (dedicated ETW field).
 * - env_time                     - Built-in ETW time at envelope level.
 * - env_dt_spanId                - OT SpanId
 * - env_dt_traceId               - OT TraceId
 * - startTime                    - OT Span start time
 * - kind                         - OT Span kind
 * - name                         - OT Span name in ETW 'Payload["name"]'
 * - parentId                     - OT Span parentId
 * - links                        - OT Span links array
 *
 * Other standard fields (reserved names) that may be appended by ETW channel:
 *
 * - Level
 * - ProviderGuid
 * - ProviderName
 * - OpcodeName
 * - KeywordName
 * - TaskName
 * - ChannelName
 * - EventMessage
 * - ActivityId
 * - Pid
 * - Tid
 *
 */
#  define ETW_FIELD_VERSION           "env_ver"               /* Event version           */
#  define ETW_FIELD_TYPE              "env_type"              /* Event type              */
#  define ETW_FIELD_NAME              "env_name"              /* Event name              */

#ifndef HAVE_FIELD_TIME
#define HAVE_FIELD_TIME
#endif
#  define ETW_FIELD_TIME              "env_time"              /* Event time at envelope  */

#  define ETW_FIELD_OPCODE            "env_opcode"            /* OpCode for TraceLogging */

#  define ETW_FIELD_TRACE_ID          "env_dt_traceId"        /* Trace Id                */
#  define ETW_FIELD_SPAN_ID           "env_dt_spanId"         /* Span Id                 */
#  define ETW_FIELD_SPAN_PARENTID     "parentId"              /* Span ParentId           */
#  define ETW_FIELD_SPAN_KIND         "kind"                  /* Span Kind               */
#  define ETW_FIELD_SPAN_LINKS        "links"                 /* Span Links array        */

#  define ETW_FIELD_PAYLOAD_NAME      "name"                  /* ETW Payload["name"]     */

/* Span option constants */
#  define ETW_FIELD_STARTTIME         "startTime"             /* Operation start time    */
#  define ETW_FIELD_ENDTTIME          "env_time"              /* Operation end time      */
#  define ETW_FIELD_DURATION          "duration"              /* Operation duration      */
#  define ETW_FIELD_STATUSCODE        "statusCode"            /* OT Span status code     */
#  define ETW_FIELD_STATUSMESSAGE     "statusMessage"         /* OT Span status message  */
#  define ETW_FIELD_SUCCESS           "success"               /* OT Span success         */
#  define ETW_FIELD_TIMESTAMP         "Timestamp"             /* Log timestamp           */

#  define ETW_FIELD_CLIENTREQID       "clientRequestId"
#  define ETW_FIELD_CORRELREQID       "correlationRequestId"

/* Value constants */
#  define ETW_VALUE_SPAN              "Span"                  /* ETW event name for Span */
#  define ETW_VALUE_LOG               "Log"                   /* ETW event name for Log */

#  define ETW_VALUE_SPAN_START        "SpanStart"             /* ETW for Span Start      */
#  define ETW_VALUE_SPAN_END          "SpanEnd"               /* ETW for Span Start      */

#  define ETW_FIELD_LOG_BODY          "body"                  /* Log body   */
#  define ETW_FIELD_LOG_SEVERITY_TEXT "severityText"          /* Sev text  */
#  define ETW_FIELD_LOG_SEVERITY_NUM  "severityNumber"        /* Sev num   */

/* clang-format on */
