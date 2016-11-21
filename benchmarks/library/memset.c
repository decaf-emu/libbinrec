/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "benchmarks/library/string.h"

void *memset(void *s, int c, unsigned long n)
{
    for (unsigned long i = 0; i < n; i++) {
        ((char *)s)[i] = (char)c;
    }
    return s;
}
