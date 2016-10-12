/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "benchmarks/library/string.h"

char *strcpy(char *dest, const char *src)
{
    char *dest_save = dest;
    while ((*dest++ = *src++) != 0) {}
    return dest_save;
}
