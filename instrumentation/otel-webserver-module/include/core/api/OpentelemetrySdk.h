/*
* Copyright 2022, OpenTelemetry Authors.  
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef OPENTELEMETRY_SDK_H
#define	OPENTELEMETRY_SDK_H

#include <stddef.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif


/* {{{ For API user: environment variables to specify Controller connectivity */
#define OTEL_SDK_ENV_OTEL_EXPORTER_TYPE "OTEL_SDK_ENV_OTEL_EXPORTER_TYPE"
#define OTEL_SDK_ENV_OTEL_EXPORTER_ENDPOINT "OTEL_SDK_ENV_OTEL_EXPORTER_ENDPOINT"             /*required*/
#define OTEL_SDK_ENV_OTEL_EXPORTER_OTLPHEADERS "OTEL_SDK_ENV_OTEL_EXPORTER_OTLPHEADERS"             /*optional*/
#define OTEL_SDK_ENV_OTEL_SSL_ENABLED "OTEL_SDK_ENV_OTEL_SSL_ENABLED"             /*optional*/
#define OTEL_SDK_ENV_OTEL_SSL_CERTIFICATE_PATH "OTEL_SDK_ENV_OTEL_SSL_CERTIFICATE_PATH"   /*optional*/
#define OTEL_SDK_ENV_OTEL_PROCESSOR_TYPE "OTEL_SDK_ENV_OTEL_PROCESSOR_TYPE"
#define OTEL_SDK_ENV_OTEL_SAMPLER_TYPE "OTEL_SDK_ENV_OTEL_SAMPLER_TYPE"
#define OTEL_SDK_ENV_OTEL_LIBRARY_NAME "OTEL_SDK_ENV_OTEL_LIBRARY_NAME"
/* {{{ For API user: optional, only if connection by aggregator is required */
#define OTEL_SDK_ENV_SERVICE_NAMESPACE "OTEL_SDK_ENV_SERVICE_NAMESPACE"
#define OTEL_SDK_ENV_SERVICE_NAME "OTEL_SDK_ENV_SERVICE_NAME"                   /*optional*/
#define OTEL_SDK_ENV_SERVICE_INSTANCE_ID "OTEL_SDK_ENV_SERVICE_INSTANCE_ID"                   /*optional*/
/* }}} */

/* {{{ For API user: environment variables to specify Webserver HostName/IP/Port */
#define OTEL_SDK_ENV_MAX_QUEUE_SIZE "OTEL_SDK_ENV_MAX_QUEUE_SIZE"                   /*required*/
#define OTEL_SDK_ENV_SCHEDULED_DELAY "OTEL_SDK_ENV_SCHEDULED_DELAY"                       /*required*/
#define OTEL_SDK_ENV_EXPORT_BATCH_SIZE "OTEL_SDK_ENV_EXPORT_BATCH_SIZE"                   /*required*/
#define OTEL_SDK_ENV_EXPORT_TIMEOUT "OTEL_SDK_ENV_EXPORT_TIMEOUT"
/* }}} */

/* {{{ For API user: environment variables to override SDK installation defaults */
#define OTEL_SDK_ENV_LOG_CONFIG_PATH "OTEL_SDK_LOG_CONFIG_PATH"
/* }}} */

// SPAN NAMING ENV VARIABLES
#define OTEL_SDK_ENV_SEGMENT_TYPE "OTEL_SDK_ENV_SEGMENT_TYPE"
#define OTEL_SDK_ENV_SEGMENT_PARAMETER "OTEL_SDK_ENV_SEGMENT_PARAMETER"


/* {{{ For API user: API User logger */
#define OTEL_LOG_API_USER_LOGGER  "api_user"    /*logging at the level of sdk function call*/
#define OTEL_LOG_API_LOGGER  "api"              /*logging at the level of sdk core functionality*/
/* }}} */

/* {{{ Internal: Status definition */
#define OTEL_PREFIXED_NAME(prefix, name) prefix##name
/* }}} */

/* {{{ For API user: Status code and error helper */
#define OTEL_STATUS(name) OTEL_PREFIXED_NAME(otel_sdk_status_,name)
#define OTEL_SUCCESS OTEL_STATUS(success)
#define OTEL_ISFAIL(value) ((value) != OTEL_SUCCESS)
#define OTEL_ISSUCCESS(value) ((value) == OTEL_SUCCESS)
/* }}} */

#if defined(_WIN32)
#define OTEL_SDK_API __declspec(dllexport)
#else
#define OTEL_SDK_API __attribute__((visibility("default")))
#endif

typedef enum OTEL_SDK_API
{
    OTEL_STATUS(success) = 0
    ,OTEL_STATUS(fail)
    ,OTEL_STATUS(agent_failed_to_start)
    ,OTEL_STATUS(unspecified_environment_variable)
    ,OTEL_STATUS(environment_variable_invalid_value)
    ,OTEL_STATUS(environment_records_are_invalid)
    ,OTEL_STATUS(environment_record_name_is_not_specified_or_empty)
    ,OTEL_STATUS(environment_record_value_is_not_specified)
    ,OTEL_STATUS(no_log_config)
    ,OTEL_STATUS(log_init_failed)
    ,OTEL_STATUS(invalid_bt_type)
    ,OTEL_STATUS(invalid_backend_type)
    ,OTEL_STATUS(bt_name_invalid)
    ,OTEL_STATUS(uninitialized)
    ,OTEL_STATUS(wrong_handle)
    ,OTEL_STATUS(cannot_create_handle)
    ,OTEL_STATUS(already_initialized)
    ,OTEL_STATUS(payload_reflector_is_null)
    ,OTEL_STATUS(backend_reflector_is_null)
    ,OTEL_STATUS(handle_pointer_is_null)
    ,OTEL_STATUS(invalid_input_string)
    ,OTEL_STATUS(uninitialized_input)
    ,OTEL_STATUS(bt_detection_disabled)
    ,OTEL_STATUS(wrong_process_id)
    ,OTEL_STATUS(cfg_channel_uninitialized)
    ,OTEL_STATUS(invalid_context)
    ,OTEL_STATUS(cannot_add_ws_context_to_core)

/* {{{ Internal: */
    ,OTEL_STATUS(count)
/* }}} */

} OTEL_SDK_STATUS_CODE;
/* }}} */

/* {{{ Internal: Handle definitions */
#define OTEL_SDK_HANDLE void*
/* }}} */

/* {{{ For API user: Handle definitions */
#define OTEL_SDK_NO_HANDLE NULL
#define OTEL_SDK_HANDLE_REQ OTEL_SDK_HANDLE
#define OTEL_SDK_HANDLE_INTERACTION OTEL_SDK_HANDLE
/* }}} */

/* {{{ For API user: function parameter value direction */
#define OTEL_SDK_PARAM_IN
#define OTEL_SDK_PARAM_OUT
/* }}} */

/* {{{ For API user: backend type */
typedef struct _OTEL_SDK_ENV_RECORD
{
    const char* name;
    const char* value;
} OTEL_SDK_ENV_RECORD;
/* }}} */

#endif	/* OPENTELEMETRY_SDK_H */
