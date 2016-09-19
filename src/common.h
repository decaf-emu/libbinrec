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

#ifndef BINREC_H
    #include "include/binrec.h"
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct RTLUnit;

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

/**
 * CODE_EXPAND_SIZE:  Sets the increment, in bytes, by which the generated
 * (host) code buffer is expanded during translation.  This is also used
 * for the initial size of the buffer.
 */
#ifndef CODE_EXPAND_SIZE
    #define CODE_EXPAND_SIZE  4096
#endif

/*************************************************************************/
/***************************** Helper macros *****************************/
/*************************************************************************/

/* Convenience macros for testing standard compliance and compiler versions.
 * major and minor must be literal integers. */
#define IS_STDC(version_date)   \
    (defined(__STDC__) && __STDC__ && __STDC_VERSION__ >= 201112)
#define IS_CLANG(major, minor)  \
    (defined(__clang__)         \
     && (__clang_major__ > major \
         || (__clang_major__ == major && __clang_minor__ >= minor)))
#define IS_GCC(major, minor)    \
    (defined(__GNUC__)          \
     && (__GNUC__ > major       \
         || (__GNUC__ == major && __GNUC_MINOR__ >= minor)))
#define IS_ICC(major, minor)    \
    (defined(__INTEL_COMPILER) && __INTEL_COMPILER >= (major * 100 + minor))
#define IS_MSVC(major, minor)   \
    (defined(_MSC_VER) && _MSC_VER >= (major * 100 + minor))

/* Wrap Clang's __has_builtin() to avoid preprocessor errors on other
 * compilers. */
#if IS_CLANG(1,0)
    #define CLANG_HAS_BUILTIN(name)  __has_builtin(name)
#else
    #define CLANG_HAS_BUILTIN(name)  0
#endif

/**
 * ALIGNED_CAST:  Cast "ptr" to pointer type "type", suppressing warnings
 * due to increased alignment.  If possible, an assertion check is made
 * that the pointer really is aligned to the required alignment.
 */
#if IS_GCC(2,95)
    #define ALIGNED_CAST(type, ptr)  __extension__({ \
        type _result = (type)(void *)(ptr); \
        ASSERT((uintptr_t)_result % __alignof__(type) == 0); \
        _result; \
    })
#else
    #define ALIGNED_CAST(type, ptr)  ((type)(void *)(ptr))
#endif

/**
 * ALWAYS_INLINE:  Function attribute indicating that the function should
 * always be inlined, even if the compiler thinks it would not be a good
 * idea.  If no compiler-specific hint is available, "inline" will be
 * substituted instead.
 */
#if IS_GCC(3,1) || IS_CLANG(1,0)
    #define ALWAYS_INLINE  inline __attribute__((always_inline))
#elif IS_MSVC(1,0)
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
    #if IS_MSVC(1,0)
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
 * FORMAT:  Function attribute indicating that the function takes a
 * printf()-type format string, allowing the compiler to typecheck the
 * format arguments.  The "fmt" argument specifies the index of the
 * function parameter containing the format string, while the "firstarg"
 * argument specifies the index of the first format argument in the
 * function parameter list (in other words, the index of the "...").
 */
#if defined(__MINGW32__)
    /* The MinGW build of GCC spits out tons of bogus warnings, so disable. */
    #define FORMAT(fmt, firstarg)  /*nothing*/
#elif IS_GCC(2,95) || IS_CLANG(1,0)
    #define FORMAT(fmt, firstarg)  __attribute__((format(printf,fmt,firstarg)))
#else
    #define FORMAT(fmt, firstarg)  /*nothing*/
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
 * NOINLINE:  Function attribute indicating that the function should never
 * be inlined, even if the compiler thinks it would be a good idea.
 */
#if IS_GCC(3,1) || IS_CLANG(1,0)
    #define NOINLINE  __attribute__((noinline))
#elif IS_MSVC(1,0)
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
 * STATIC_ASSERT:  Verify that the given condition is true at compile time,
 * and generate a compilation error if it is not.  These assertions are
 * always enabled, regardless of whether ENABLE_ASSERT is defined.
 */
#if IS_STDC(201112) || IS_GCC(4,6)
    #define STATIC_ASSERT(expr, message)  _Static_assert(expr, message)
#elif IS_MSVC(1,0)
    #define STATIC_ASSERT(expr, message)  static_assert(expr, message)
#else
    #define STATIC_ASSERT(expr, message)  do { \
        struct static_assert { \
            int _error_if_negative : 1 - 2 * ((expr) == 0); \
        }; \
    } while (0)
#endif

/**
 * UNREACHABLE:  Compiler intrinsic indicating that the current code
 * location can never be reached.
 */
#if IS_GCC(4,5) || IS_CLANG(2,7)
    #define UNREACHABLE  __builtin_unreachable()
#elif IS_MSVC(1,0)
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

/**
 * align_up:  Align the given value (x) up to a multiple of "align" bytes.
 */
static CONST_FUNCTION inline uintptr_t align_up(uintptr_t x, unsigned int align)
    {return (x + (align-1)) / align * align;}

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
#define READONLY_PAGE_SIZE  (1u << READONLY_PAGE_BITS)
#define READONLY_PAGE_MASK  (READONLY_PAGE_SIZE - 1)

/*-----------------------------------------------------------------------*/

struct binrec_t {

    /* Handle configuration, as passed to binrec_create(). */
    binrec_setup_t setup;

    /* Flag indicating host architecture's endianness. */
    bool host_little_endian;

    /* Buffer for generated code. */
    uint8_t *code_buffer;
    /* Allocated size of the code buffer, in bytes. */
    long code_buffer_size;
    /* Number of bytes of actual code stored in the buffer. */
    long code_len;
    /* Alignment to use when (re)allocating the code buffer. */
    size_t code_alignment;

    /* Valid address range (inclusive) for code translation. */
    uint32_t code_range_start;
    uint32_t code_range_end;

    /* Current set of optimization flags. */
    unsigned int common_opt;
    unsigned int guest_opt;
    unsigned int host_opt;

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

/*-------------------------- Logging routines ---------------------------*/

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
extern void log_info(binrec_t *handle, const char *format, ...) FORMAT(2, 3);
#define log_warning INTERNAL(log_warning)
extern void log_warning(binrec_t *handle, const char *format, ...) FORMAT(2, 3);
#define log_error INTERNAL(log_error)
extern void log_error(binrec_t *handle, const char *format, ...) FORMAT(2, 3);

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

/*--------------------- Memory allocation routines ----------------------*/

/**
 * binrec_malloc:  Allocate general-purpose memory.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     size: Size of memory to allocate, in bytes.
 * [Return value]
 *     Allocated memory block, or NULL on error.
 */
static inline void *binrec_malloc(const binrec_t *handle, size_t size)
{
    ASSERT(handle);

    if (handle->setup.malloc) {
        return (*handle->setup.malloc)(handle->setup.userdata, size);
    } else {
        return malloc(size);
    }
}

/**
 * binrec_realloc:  Resize a general-purpose memory block.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     ptr: Memory block to resize.
 *     size: New size of memory block, in bytes.
 * [Return value]
 *     Resized memory block, or NULL on error.
 */
static inline void *binrec_realloc(const binrec_t *handle,
                                   void *ptr, size_t size)
{
    ASSERT(handle);

    if (handle->setup.realloc) {
        return (*handle->setup.realloc)(handle->setup.userdata, ptr, size);
    } else {
        return realloc(ptr, size);
    }
}

/**
 * binrec_free:  Free general-purpose memory.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     ptr: Memory block to free.
 */
static inline void binrec_free(const binrec_t *handle, void *ptr)
{
    ASSERT(handle);

    if (handle->setup.free) {
        (*handle->setup.free)(handle->setup.userdata, ptr);
    } else {
        free(ptr);
    }
}

/**
 * binrec_expand_code_buffer:  Expand the code buffer to at least the
 * given size.
 *
 * Translators should normally call binrec_ensure_code_space() instead of
 * this function.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     new_size: New buffer size (must be greater than the current size).
 * [Return value]
 *     True on success, false if not enough memory was available.
 */
#define binrec_expand_code_buffer INTERNAL(binrec_expand_code_buffer)
extern bool binrec_expand_code_buffer(binrec_t *handle, long new_size);

/**
 * binrec_ensure_code_space:  Ensure that the given number of bytes are
 * available past the current output code length in the handle's output
 * code buffer.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     bytes: Number of bytes required.
 * [Return value]
 *     True on success, false if not enough memory was available.
 */
static inline bool binrec_ensure_code_space(binrec_t *handle, long bytes)
{
    if (LIKELY(handle->code_len + bytes <= handle->code_buffer_size)) {
        return true;
    }
    return binrec_expand_code_buffer(handle, handle->code_len + bytes);
}

/*----------------------------------*/

/* Ensure that code always calls one of the wrappers above rather than
 * malloc()/realloc()/free() directly. */
#define malloc  _invalid_call_to_malloc
#define realloc _invalid_call_to_realloc
#define free    _invalid_call_to_free

/*************************************************************************/
/*************************************************************************/

#endif  /* SRC_COMMON_H */
