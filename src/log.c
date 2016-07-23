/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include "src/common.h"

#include <stdarg.h>
#include <stdio.h>


static void do_log(binrec_t *handle, binrec_loglevel_t level,
                   const char *format, va_list args)
{
    if (!handle->setup.log) {
        return;
    }

    char buffer[1000];
    if (vsnprintf(buffer, sizeof(buffer), format, args) >= (int)sizeof(buffer)) {
        (*handle->setup.log)(handle->setup.userdata, BINREC_LOGLEVEL_WARNING,
                             "Next log message is truncated");
    }
    (*handle->setup.log)(handle->setup.userdata, level, buffer);
}


void INTERNAL(log_info)(binrec_t *handle, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    do_log(handle, BINREC_LOGLEVEL_INFO, format, args);
    va_end(args);
}


void INTERNAL(log_warning)(binrec_t *handle, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    do_log(handle, BINREC_LOGLEVEL_WARNING, format, args);
    va_end(args);
}


void INTERNAL(log_error)(binrec_t *handle, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    do_log(handle, BINREC_LOGLEVEL_ERROR, format, args);
    va_end(args);
}
