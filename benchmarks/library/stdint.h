/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BENCHMARKS_LIBRARY_STDINT_H
#define BENCHMARKS_LIBRARY_STDINT_H

/*
 * This header supplies a basic <stdint.h> for use when it is not available
 * to the compiler.
 */

typedef long long int64_t;
#define INT64_C(n) n##LL

typedef unsigned long long uint64_t;
#define UINT64_C(n) n##ULL

#endif  /* BENCHMARKS_LIBRARY_STRING_H */
