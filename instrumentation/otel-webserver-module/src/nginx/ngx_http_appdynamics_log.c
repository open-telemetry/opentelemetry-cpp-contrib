/*
    Copyright 2020 AppDynamics.
    All rights reserved.
*/

#include "ngx_http_appdynamics_log.h"

ngx_flag_t logState = false;

/* This will log the error only when logState(traceAsError) flag is set by user*/
void ngx_writeTrace(ngx_log_t *log, const char* funcName, const char* format, ...)
{
    char note[8192];
    va_list args;

    va_start(args, format);
    vsnprintf(note, sizeof(note), format, args);
    va_end(args);

    if (logState && log)
    {
        ngx_log_error(NGX_LOG_ERR, log, 0, "mod_appdynamics: %s: %s", funcName, note);
    }
}

/* This will always log the error irrespective of the logState(traceAsError) flag */
void ngx_writeError(ngx_log_t *log, const char* funcName, const char* format, ...)
{
    char note[8192];
    va_list args;

    va_start(args, format);
    vsnprintf(note, sizeof(note), format, args);
    va_end(args);

    if (log && funcName)
    {
        ngx_log_error(NGX_LOG_ERR, log, 0, "mod_appdynamics: %s: %s", funcName, note);
    }
}
