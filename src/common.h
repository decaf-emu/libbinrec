/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef SRC_COMMON_H
#define SRC_COMMON_H

#include <stdbool.h>

/*************************************************************************/
/***************************** Helper macros *****************************/
/*************************************************************************/

/* Convenience macros for testing compiler versions.  major and minor must
 * be literal integers. */
#define IS_CLANG(major,minor)   \
    defined(__clang__)          \
    && (__clang_major__ > major \
        || (__clang_major__ == major && __clang_minor__ >= minor))
#define IS_GCC(major,minor)     \
    defined(__GNUC__)           \
    && (__GNUC__ > major        \
        || (__GNUC__ == major && __GNUC_MINOR__ >= minor))

/**
 * ASSERT:  Verify that the given condition is true, and abort the program
 * if it is not.  If ENABLE_ASSERT is not defined, the expression is
 * evaluated and its result is discarded; if supported by the compiler, a
 * hint is provided that the expression is always true.
 */
#ifdef ENABLE_ASSERT
# include <assert.h>
# define ASSERT  assert
#else
# define ASSERT(expr)  do {if (UNLIKELY(!(expr))) {UNREACHABLE;}} while (0)
#endif

/**
 * COLD_FUNCTION:  Function attribute indicating that the given function
 * is not expected to be called often.
 */
#if IS_GCC(4,3) || IS_CLANG(3,2)
# define COLD_FUNCTION  __attribute__((cold))
#else
# define COLD_FUNCTION  /*nothing*/
#endif

/**
 * CONST_FUNCTION:  Function attribute indicating that the function's
 * behavior depends only on its arguments and the function has no side
 * effects.
 */
#if IS_GCC(2,95) || IS_CLANG(1,0)
# define CONST_FUNCTION  __attribute__((const))
#else
# define CONST_FUNCTION  /*nothing*/
#endif

/**
 * PURE_FUNCTION:  Function attribute indicating that the function's
 * behavior depends only on its arguments and global state, and the
 * function has no side effects.
 */
#if IS_GCC(3,0) || IS_CLANG(1,0)
# define PURE_FUNCTION  __attribute__((pure))
#else
# define PURE_FUNCTION  /*nothing*/
#endif

/**
 * LIKELY, UNLIKELY:  Construct which indicates to the compiler that the
 * given expression is likely or unlikely to evaluate to true.
 */
#if IS_GCC(3,0) || IS_CLANG(1,0)
# define LIKELY(expr)    (__builtin_expect(!!(expr), 1))
# define UNLIKELY(expr)  (__builtin_expect(!!(expr), 0))
#else
# define LIKELY(expr)    (expr)
# define UNLIKELY(expr)  (expr)
#endif

/**
 * UNREACHABLE:  Compiler intrinsic indicating that the current code
 * location can never be reached.
 */
#if IS_GCC(4,5) || IS_CLANG(2,7)
# define UNREACHABLE  __builtin_unreachable()
#else
# define UNREACHABLE  /*nothing*/
#endif

/**
 * UNUSED:  Attribute indicating that a definition is intentionally unused.
 */
#if IS_GCC(2,95) || IS_CLANG(1,0)
# define UNUSED  __attribute__((unused))
#else
# define UNUSED  /*nothing*/
#endif

/**
 * min, max:  Return the minimum or maximum of two values.  The returned
 * value will be evaluated twice.
 */
#define min(a,b)  ((a) < (b) ? (a) : (b))
#define max(a,b)  ((a) > (b) ? (a) : (b))

/**
 * lenof:  Return the length of (number of elements in) the given array.
 */
#define lenof(array)  ((int)(sizeof((array)) / sizeof(*(array))))

/*-----------------------------------------------------------------------*/

/**
 * INTERNAL:  Prepend "_libbinrec_" to the given symbol name.  Used to
 * avoid clashes between internal libbinrec functions shared between
 * multiple source files and symbols in client code.
 *
 * This symbol renaming can be disabled by compiling with "-DINTERNAL(x)=x",
 * which may make debugging easier.
 */
#ifndef INTERNAL
# define INTERNAL(name)  _libbinrec_##name
#endif

/*************************************************************************/
/*********************** Data types and constants ************************/
/*************************************************************************/

/*************************************************************************/
/********************** Internal utility functions ***********************/
/*************************************************************************/

/*************************************************************************/
/*************************************************************************/

#endif  /* SRC_COMMON_H */
