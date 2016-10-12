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


static void vdo_log(binrec_t *handle, binrec_loglevel_t level,
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


static void do_log(binrec_t *handle, binrec_loglevel_t level,
                   const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vdo_log(handle, level, format, args);
    va_end(args);
}


void INTERNAL(log_info)(binrec_t *handle, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vdo_log(handle, BINREC_LOGLEVEL_INFO, format, args);
    va_end(args);
}


void INTERNAL(log_warning)(binrec_t *handle, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vdo_log(handle, BINREC_LOGLEVEL_WARNING, format, args);
    va_end(args);
}


void INTERNAL(log_error)(binrec_t *handle, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vdo_log(handle, BINREC_LOGLEVEL_ERROR, format, args);
    va_end(args);
}


void INTERNAL(_log_ice)(binrec_t *handle, const char *file, int line,
                        const char *format, ...)
{
    const char *pathsep;
#ifdef _WIN32
    pathsep = "/\\";
#else
    pathsep = "/";
#endif
    while (file[strcspn(file, pathsep)]) {
        file += strcspn(file, pathsep) + 1;
    }

    char buf[1000];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    do_log(handle, BINREC_LOGLEVEL_ERROR, "Internal compiler error: %s:%d: %s",
           file, line, buf);
}
