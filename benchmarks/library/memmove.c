/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "benchmarks/library/string.h"

void *memmove(void *dest, const void *src, unsigned long n)
{
    if ((const char *)dest <= (const char *)src) {
        for (unsigned long i = 0; i < n; i++) {
            ((char *)dest)[i] = ((char *)src)[i];
        }
    } else {
        for (unsigned long i = n; i > 0; i--) {
            ((char *)dest)[i-1] = ((char *)src)[i-1];
        }
    }
    return dest;
}
