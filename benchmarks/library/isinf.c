/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "benchmarks/library/math.h"
#include "benchmarks/library/stdint.h"

int isinf(double x)
{
    union {double f; uint64_t i;} u;
    u.f = x;
    return (u.i << 1) == (UINT64_C(0x7FF0000000000000) << 1);
}
