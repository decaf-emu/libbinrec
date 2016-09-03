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

#include "include/binrec.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************/
/************************ Configuration constants ************************/
/*************************************************************************/

/**
 * READONLY_PAGE_BITS:  Sets the (number of bits in the) page size used
 * for tracking read-only pages.  Larger values reduce the size of the
 * full-page map and thus increase the data locality of page bit tests,
 * but if the page size is set larger than the alignment of read-only
 * regions, the ends of the regions will consume partial-page map entries,
 * which increases overhead during translation.
 *
 * The default value of 12 gives a page size of 4k (0x1000 bytes) and
 * requires 256k of memory per handle.
 */
#ifndef READONLY_PAGE_BITS
    #define READONLY_PAGE_BITS  12
#endif

/**
 * MAX_PARTIAL_READONLY:  Sets the maximum number of partially read-only
 * pages to track.  Each entry requires 1 bit of memory per byte in the
 * page size (plus 4 bytes for the page's base address).
 *
 * The default value of 64, if used with the default READONLY_PAGE_BITS
 * value of 12, requires approximately 32k of memory per handle.
 */
#ifndef MAX_PARTIAL_READONLY
    #define MAX_PARTIAL_READONLY  64
#endif

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
 * ALWAYS_INLINE:  Function attribute indicating that the function should
 * always be inlined, even if the compiler thinks it would not be a good
 * idea.  If no compiler-specific hint is available, "inline" will be
 * substituted instead.
 */
#if IS_GCC(3,1) || IS_CLANG(1,0)
    #define ALWAYS_INLINE  inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define ALWAYS_INLINE  __forceinline
#else
    #define ALWAYS_INLINE  inline
#endif

/**
 * ASSERT:  Verify that the given condition is true, and abort the program
 * if it is not.  If ENABLE_ASSERT is not defined, the expression is
 * evaluated and its result is discarded; if supported by the compiler, a
 * hint is provided that the expression is always true.
 */
#ifdef ENABLE_ASSERT
    #include <assert.h>
    #define ASSERT  assert
#else
    #if defined(_MSC_VER)
        #define ASSERT(expr)  __assume((expr))
    #else
        #define ASSERT(expr)  do {if (UNLIKELY(!(expr))) {UNREACHABLE;}} while (0)
    #endif
#endif

/**
 * COLD_FUNCTION:  Function attribute indicating that the given function
 * is not expected to be called often.
 */
#if IS_GCC(4,3) || IS_CLANG(3,2)
    #define COLD_FUNCTION  __attribute__((cold))
#else
    #define COLD_FUNCTION  /*nothing*/
#endif

/**
 * CONST_FUNCTION:  Function attribute indicating that the function's
 * behavior depends only on its arguments and the function has no side
 * effects.
 */
#if IS_GCC(2,95) || IS_CLANG(1,0)
    #define CONST_FUNCTION  __attribute__((const))
#else
    #define CONST_FUNCTION  /*nothing*/
#endif

/**
 * NOINLINE:  Function attribute indicating that the function should never
 * be inlined, even if the compiler thinks it would be a good idea.
 */
#if IS_GCC(3,1) || IS_CLANG(1,0)
    #define NOINLINE  __attribute__((noinline))
#elif defined(_MSC_VER)
    #define NOINLINE  __declspec(noinline)
#else
    #define NOINLINE  /*nothing*/
#endif

/**
 * PURE_FUNCTION:  Function attribute indicating that the function's
 * behavior depends only on its arguments and global state, and the
 * function has no side effects.
 */
#if IS_GCC(3,0) || IS_CLANG(1,0)
    #define PURE_FUNCTION  __attribute__((pure))
#else
    #define PURE_FUNCTION  /*nothing*/
#endif

/**
 * LIKELY, UNLIKELY:  Construct which indicates to the compiler that the
 * given expression is likely or unlikely to evaluate to true.
 */
#if IS_GCC(3,0) || IS_CLANG(1,0)
    #define LIKELY(expr)    (__builtin_expect(!!(expr), 1))
    #define UNLIKELY(expr)  (__builtin_expect(!!(expr), 0))
#else
    #define LIKELY(expr)    (expr)
    #define UNLIKELY(expr)  (expr)
#endif

/**
 * UNREACHABLE:  Compiler intrinsic indicating that the current code
 * location can never be reached.
 */
#if IS_GCC(4,5) || IS_CLANG(2,7)
    #define UNREACHABLE  __builtin_unreachable()
#elif defined(_MSC_VER)
    #define UNREACHABLE  __assume(0)
#else
    #define UNREACHABLE  /*nothing*/
#endif

/**
 * UNUSED:  Attribute indicating that a definition is intentionally unused.
 */
#if IS_GCC(2,95) || IS_CLANG(1,0)
    #define UNUSED  __attribute__((unused))
#else
    #define UNUSED  /*nothing*/
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
    #define INTERNAL(name)  _libbinrec_##name
#endif

/*************************************************************************/
/*********************** Data types and constants ************************/
/*************************************************************************/

/* Convenience constants for read-only page management. */
#define READONLY_PAGE_SIZE  (1U << READONLY_PAGE_BITS)
#define READONLY_PAGE_MASK  (READONLY_PAGE_SIZE - 1)

/*-----------------------------------------------------------------------*/

struct binrec_t {

    /* Handle configuration, as passed to binrec_create().  If the memory
     * allocation functions were specified as NULL, they are filled in here
     * with default functions. */
    binrec_setup_t setup;

    /* Valid address range (inclusive) for code translation. */
    uint32_t code_range_start;
    uint32_t code_range_end;

    /* Current set of optimization flags. */
    unsigned int optimizations;

    /* Settings for inlining. */
    int max_inline_length;
    int max_inline_depth;

    /* Map of read-only pages within the guest address space.  Two bits are
     * allocated to each page; the higher-order bit indicates that the
     * entire page is read-only, and the lower-order bit indicates that
     * part but not all of the page is read-only.  Within a byte, the
     * lowest-order pair of bits corresponds to the lowest memory address. */
    uint8_t readonly_map[1 << (32 - READONLY_PAGE_BITS - 2)];
    /* Map of partially read-only pages within the guest address space.
     * partial_readonly_pages[] gives the base address of a page, and the
     * corresponding entry in partial_readonly_map[] indicates the
     * read-only status of each byte within the page, with one bit per
     * byte.  The arrays are kept sorted by page address; an address of ~0
     * indicates that all remaining entries are unused. */
    uint32_t partial_readonly_pages[MAX_PARTIAL_READONLY];
    uint8_t partial_readonly_map[MAX_PARTIAL_READONLY][1 << (READONLY_PAGE_BITS - 3)];

};

/*************************************************************************/
/********************** Internal utility functions ***********************/
/*************************************************************************/

/**
 * log_info, log_warning, log_error:  Log an informational message, warning,
 * or error, respectively.
 *
 * [Parameters]
 *     handle: Current handle.
 *     format: printf()-style format string.
 *     ...: Format arguments.
 */
#define log_info INTERNAL(log_info)
extern void log_info(binrec_t *handle, const char *format, ...);
#define log_warning INTERNAL(log_warning)
extern void log_warning(binrec_t *handle, const char *format, ...);
#define log_error INTERNAL(log_error)
extern void log_error(binrec_t *handle, const char *format, ...);

/**
 * log_ice:  Log an internal compiler error, including the source file and
 * line number at which the error occurred.
 *
 * [Parameters]
 *     handle: Current handle.
 *     format: printf()-style format string.  Must be a stirng literal.
 *     ...: Format arguments.
 */
#define _log_ice INTERNAL(_log_ice)
extern void _log_ice(binrec_t *handle, const char *file, int line,
                     const char *format, ...);
#define log_ice(handle, ...)  \
    _log_ice((handle), __FILE__, __LINE__, __VA_ARGS__)

/*************************************************************************/
/*************************************************************************/

#endif  /* SRC_COMMON_H */
