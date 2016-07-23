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
 * requires 
 */
#ifndef READONLY_PAGE_BITS
# define READONLY_PAGE_BITS  12
#endif

/**
 * MAX_PARTIAL_READONLY:  Sets the maximum number of partially read-only
 * pages to track.  Each entry requires 1 bit of memory per byte in the
 * page size (plus 4 bytes for the page's base address).
 */
#ifndef MAX_PARTIAL_READONLY
# define MAX_PARTIAL_READONLY  64
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

/* Convenience constants for read-only page management. */
#define READONLY_PAGE_SIZE  (1U << READONLY_PAGE_BITS)
#define READONLY_PAGE_MASK  (READONLY_PAGE_SIZE - 1)

/*-----------------------------------------------------------------------*/

struct binrec_t {

    /* Handle configuration, as passed to binrec_create().  If the memory
     * allocation functions were specified as NULL, they are filled in here
     * with default functions. */
    binrec_setup_t setup;

    /* Current set of optimization flags. */
    unsigned int optimizations;

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

/*************************************************************************/
/*************************************************************************/

#endif  /* SRC_COMMON_H */
