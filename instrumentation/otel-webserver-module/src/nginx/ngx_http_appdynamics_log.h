/*
    Copyright 2021 AppDynamics.
    All rights reserved.
*/

#include <stdbool.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <stdarg.h>

/* 
	To log Agent logs into NGINX error logs, 
	based on the flag "AppDynamicsTraceAsError" set by user 
*/
ngx_flag_t logState; // read the value of "AppDynamicsTraceAsError" flag
void ngx_writeTrace(ngx_log_t *log, const char* funcName, const char* note, ...);
void ngx_writeError(ngx_log_t *log, const char* funcName, const char* note, ...); 
