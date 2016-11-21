/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BENCHMARKS_LIBRARY_MATH_INTERNAL_H
#define BENCHMARKS_LIBRARY_MATH_INTERNAL_H

#include "stdint.h"

#define bool int
#define false 0
#define true 1

#define MIN(a,b) ((a) < (b) ? (a) : (b))

extern int __branred(double x, double *a, double *aa);
extern double __cos(double x);
extern double __cos32(double x, double res, double res1);
extern void __docos(double x, double dx, double v[]);
extern void __dubcos(double x, double dx, double v[]);
extern void __dubsin(double x, double dx, double v[]);
extern double __exp1(double x, double xx, double error);
extern double __ieee754_exp(double x);
extern double __ieee754_log(double x);
extern double __ieee754_sqrt(double x);
extern double __mpcos(double x, double dx, bool reduce_range);
extern double __mpsin(double x, double dx, bool reduce_range);
extern double __sin(double x);
extern double __sin32(double x, double res, double res1);
extern double __slowexp(double x);

static inline double __copysign(double x, double sign) {
    union {double f; uint64_t i;} u, v;
    u.f = x;
    v.f = sign;
    u.i &= ~(UINT64_C(1) << 63);
    u.i |= v.i & (UINT64_C(1) << 63);
    return u.f;
}

#define __set_errno(...) (void)0
#define SET_RESTORE_ROUND(...) (void)0
#define SET_RESTORE_ROUND_53BIT(...) (void)0
#define LIBC_PROBE(...)
#define attribute_hidden

#if defined(__GNUC__)
    #define math_force_eval(x) __asm__ volatile("" : : "X" (x))
#else
    #define math_force_eval(x) (x)
#endif

#define math_check_force_underflow(x)  do { \
    const double min_normal = 2.2250738585072014e-308; \
    if (fabs(x) < min_normal) { \
        double force_underflow = x * x; \
        math_force_eval(force_underflow); \
    } \
} while (0)

#define math_check_force_underflow_nonneg(x)  do { \
    const double min_normal = 2.2250738585072014e-308; \
    if (x < min_normal) { \
        double force_underflow = x * x; \
        math_force_eval(force_underflow); \
    } \
} while (0)

#undef __always_inline
#if defined(__GNUC__)
    #define __always_inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define __always_inline  __forceinline
#else
    #define __always_inline  inline
#endif

#undef __glibc_likely
#undef __glibc_unlikely
#if defined(__GNUC__)
    #define __glibc_likely(x) __builtin_expect(!!(x), 1)
    #define __glibc_unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define __glibc_likely(x) (x)
    #define __glibc_unlikely(x) (x)
#endif

#endif  /* BENCHMARKS_LIBRARY_MATH_INTERNAL_H */
