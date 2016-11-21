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

double fabs(double x)
{
#if defined(__GNUC__) && defined(__ppc__)
    __asm__("fabs %0,%1" : "=d" (x) : "d" (x));
    return x;
#else
    union {double f; uint64_t i;} u;
    u.f = x;
    u.i &= ~(UINT64_C(1) << 63);
    return u.f;
#endif
}
