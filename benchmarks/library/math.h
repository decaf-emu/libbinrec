/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BENCHMARKS_LIBRARY_MATH_H
#define BENCHMARKS_LIBRARY_MATH_H

/*
 * This header provides declarations of commonly used <math.h> functions
 * for use when they are not available to the compiler.  The actual
 * functions are each defined in separate source files in this directory,
 * and their implementations (borrowed from glibc) can be found in the
 * math/ subdirectory.
 */

extern double atan(double x);
extern double cos(double x);
extern double exp(double x);
extern double fabs(double x);
extern double log(double x);
extern double sin(double x);
extern double sqrt(double x);

/* This should theoretically be a generic macro, but a single function is
 * good enough for our purposes. */
extern int isinf(double x);

#endif  /* BENCHMARKS_LIBRARY_MATH_H */
