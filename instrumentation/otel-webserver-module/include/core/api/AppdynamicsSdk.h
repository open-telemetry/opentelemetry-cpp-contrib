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

#ifndef APPDYNAMICS_SDK_H
#define	APPDYNAMICS_SDK_H

#include <stddef.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif


/* {{{ For API user: environment variables to specify Controller connectivity */
#define APPD_SDK_ENV_OTEL_EXPORTER_TYPE "APPD_SDK_ENV_OTEL_EXPORTER_TYPE"
#define APPD_SDK_ENV_OTEL_EXPORTER_ENDPOINT "APPD_SDK_ENV_OTEL_EXPORTER_ENDPOINT"             /*required*/
#define APPD_SDK_ENV_OTEL_SSL_ENABLED "APPD_SDK_ENV_OTEL_SSL_ENABLED"             /*optional*/
#define APPD_SDK_ENV_OTEL_SSL_CERTIFICATE_PATH "APPD_SDK_ENV_OTEL_SSL_CERTIFICATE_PATH"   /*optional*/
#define APPD_SDK_ENV_OTEL_PROCESSOR_TYPE "APPD_SDK_ENV_OTEL_PROCESSOR_TYPE"
#define APPD_SDK_ENV_OTEL_SAMPLER_TYPE "APPD_SDK_ENV_OTEL_SAMPLER_TYPE"
#define APPD_SDK_ENV_OTEL_LIBRARY_NAME "APPD_SDK_ENV_OTEL_LIBRARY_NAME"
/* {{{ For API user: optional, only if connection by aggregator is required */
#define APPD_SDK_ENV_SERVICE_NAMESPACE "APPD_SDK_ENV_SERVICE_NAMESPACE"
#define APPD_SDK_ENV_SERVICE_NAME "APPD_SDK_ENV_SERVICE_NAME"                   /*optional*/
#define APPD_SDK_ENV_SERVICE_INSTANCE_ID "APPD_SDK_ENV_SERVICE_INSTANCE_ID"                   /*optional*/
/* }}} */

/* {{{ For API user: environment variables to specify Webserver HostName/IP/Port */
#define APPD_SDK_ENV_MAX_QUEUE_SIZE "APPD_SDK_ENV_MAX_QUEUE_SIZE"                   /*required*/
#define APPD_SDK_ENV_SCHEDULED_DELAY "APPD_SDK_ENV_SCHEDULED_DELAY"                       /*required*/
#define APPD_SDK_ENV_EXPORT_BATCH_SIZE "APPD_SDK_ENV_EXPORT_BATCH_SIZE"                   /*required*/
#define APPD_SDK_ENV_EXPORT_TIMEOUT "APPD_SDK_ENV_EXPORT_TIMEOUT"
/* }}} */

/* {{{ For API user: environment variables to override SDK installation defaults */
#define APPD_SDK_ENV_LOG_CONFIG_PATH "APPD_SDK_LOG_CONFIG_PATH"
/* }}} */

// SPAN NAMING ENV VARIABLES
#define APPD_SDK_ENV_SEGMENT_TYPE "APPD_SDK_ENV_SEGMENT_TYPE"
#define APPD_SDK_ENV_SEGMENT_PARAMETER "APPD_SDK_ENV_SEGMENT_PARAMETER"

/* {{{ For API user: API User logger */
#define APPD_LOG_API_USER_LOGGER  "api_user"    /*logging at the level of sdk function call*/
#define APPD_LOG_API_LOGGER  "api"              /*logging at the level of sdk core functionality*/
/* }}} */

/* {{{ Internal: Status definition */
#define APPD_PREFIXED_NAME(prefix, name) prefix##name
/* }}} */

/* {{{ For API user: Status code and error helper */
#define APPD_STATUS(name) APPD_PREFIXED_NAME(appd_sdk_status_,name)
#define APPD_SUCCESS APPD_STATUS(success)
#define APPD_ISFAIL(value) ((value) != APPD_SUCCESS)
#define APPD_ISSUCCESS(value) ((value) == APPD_SUCCESS)
/* }}} */

#if defined(_WIN32)
#define APPD_SDK_API __declspec(dllexport)
#else
#define APPD_SDK_API __attribute__((visibility("default")))
#endif

typedef enum APPD_SDK_API
{
    APPD_STATUS(success) = 0
    ,APPD_STATUS(fail)
    ,APPD_STATUS(agent_failed_to_start)
    ,APPD_STATUS(unspecified_environment_variable)
    ,APPD_STATUS(environment_variable_invalid_value)
    ,APPD_STATUS(environment_records_are_invalid)
    ,APPD_STATUS(environment_record_name_is_not_specified_or_empty)
    ,APPD_STATUS(environment_record_value_is_not_specified)
    ,APPD_STATUS(no_log_config)
    ,APPD_STATUS(log_init_failed)
    ,APPD_STATUS(invalid_bt_type)
    ,APPD_STATUS(invalid_backend_type)
    ,APPD_STATUS(bt_name_invalid)
    ,APPD_STATUS(uninitialized)
    ,APPD_STATUS(wrong_handle)
    ,APPD_STATUS(cannot_create_handle)
    ,APPD_STATUS(already_initialized)
    ,APPD_STATUS(payload_reflector_is_null)
    ,APPD_STATUS(backend_reflector_is_null)
    ,APPD_STATUS(handle_pointer_is_null)
    ,APPD_STATUS(invalid_input_string)
    ,APPD_STATUS(uninitialized_input)
    ,APPD_STATUS(bt_detection_disabled)
    ,APPD_STATUS(wrong_process_id)
    ,APPD_STATUS(cfg_channel_uninitialized)
    ,APPD_STATUS(invalid_context)
    ,APPD_STATUS(cannot_add_ws_context_to_core)

/* {{{ Internal: */
    ,APPD_STATUS(count)
/* }}} */

} APPD_SDK_STATUS_CODE;
/* }}} */

/* {{{ Internal: Handle definitions */
#define APPD_SDK_HANDLE void*
/* }}} */

/* {{{ For API user: Handle definitions */
#define APPD_SDK_NO_HANDLE NULL
#define APPD_SDK_HANDLE_REQ APPD_SDK_HANDLE
#define APPD_SDK_HANDLE_INTERACTION APPD_SDK_HANDLE
/* }}} */

/* {{{ For API user: function parameter value direction */
#define APPD_SDK_PARAM_IN
#define APPD_SDK_PARAM_OUT
/* }}} */

/* {{{ For API user: backend type */
typedef struct _APPD_SDK_ENV_RECORD
{
    const char* name;
    const char* value;
} APPD_SDK_ENV_RECORD;
/* }}} */

#endif	/* APPDYNAMICS_SDK_H */
