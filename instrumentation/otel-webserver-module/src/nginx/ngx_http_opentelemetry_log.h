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


#include <ngx_config.h>
#include <ngx_core.h>
#include <stdbool.h>
#include <stdarg.h>

/* 
	To log Agent logs into NGINX error logs, 
	based on the flag "NginxModuleTraceAsError" set by user
*/
ngx_flag_t logState; // read the value of "NginxModuleTraceAsError" flag
void ngx_writeTrace(ngx_log_t *log, const char* funcName, const char* note, ...);
void ngx_writeError(ngx_log_t *log, const char* funcName, const char* note, ...); 
