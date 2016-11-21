/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BENCHMARKS_LIBRARY_STRING_H
#define BENCHMARKS_LIBRARY_STRING_H

/*
 * This header provides declarations of commonly used <string.h> functions
 * for use when they are not available to the compiler.  The actual
 * functions are each defined in separate source files in this directory.
 */

extern void *memcpy(void *dest, const void *src, unsigned long n);
extern void *memmove(void *dest, const void *src, unsigned long n);
extern void *memset(void *s, int c, unsigned long n);
extern int strcmp(const char *a, const char *b);
extern char *strcpy(char *dest, const char *src);

#endif  /* BENCHMARKS_LIBRARY_STRING_H */
