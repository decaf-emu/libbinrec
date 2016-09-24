/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef TESTS_COMMON_H
#define TESTS_COMMON_H

#ifndef SRC_COMMON_H
    #include "src/common.h"
#endif

/* Disable malloc() suppression from src/common.h. */
#undef malloc
#undef realloc
#undef free

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************/
/**************** Macros for checking conditions in tests ****************/
/*************************************************************************/

/**
 * FAIL:  Log the given error message along with the source file and line,
 * and fail the current test.  Pass a format string and format arguments as
 * for printf().
 */
#define FAIL(...)  do {                                                 \
    fprintf(stderr, "%s:%d: ", __FILE__, __LINE__);                     \
    fprintf(stderr, __VA_ARGS__);                                       \
    fputc('\n', stderr);                                                \
    return EXIT_FAILURE;                                                \
} while (0)

/*-----------------------------------------------------------------------*/

/**
 * EXPECT:  Check that the given expression is true, and fail the test if not.
 */
#define EXPECT(expr)  do {                                              \
    if (!(expr)) {                                                      \
        FAIL("%s was not true as expected", #expr);                     \
    }                                                                   \
} while (0)

/*-----------------------------------------------------------------------*/

/**
 * EXPECT_FALSE:  Check that the given expression is false, and fail the
 * test if not.
 */
#define EXPECT_FALSE(expr)  do {                                        \
    if ((expr)) {                                                       \
        FAIL("%s was not false as expected", #expr);                     \
    }                                                                   \
} while (0)

/*-----------------------------------------------------------------------*/

/**
 * EXPECT_EQ:  Check that the given integer expression is equal to an
 * expected value, and fail the test if not.
 */
#define EXPECT_EQ(expr, value)  do {                                    \
    const intmax_t _expr = (expr);                                      \
    const intmax_t _value = (value);                                    \
    if (!(_expr == _value)) {                                           \
        FAIL("%s was %jd but should have been %jd", #expr, _expr, _value); \
    }                                                                   \
} while (0)

/*-----------------------------------------------------------------------*/

/**
 * EXPECT_GT:  Check that the given integer expression is greather than an
 * expected value, and fail the test if not.
 */
#define EXPECT_GT(expr, value)  do {                                    \
    const intmax_t _expr = (expr);                                      \
    const intmax_t _value = (value);                                    \
    if (!(_expr > _value)) {                                            \
        FAIL("%s was %jd but should have been greater than %jd",      \
             #expr, _expr, _value);                                     \
    }                                                                   \
} while (0)

/*-----------------------------------------------------------------------*/

/**
 * EXPECT_FLTEQ:  Check that the given floating-point expression is equal
 * to an expected value, and fail the test if not.
 */
#define EXPECT_FLTEQ(expr, value)  do {                                 \
    const long double _expr = (expr);                                   \
    const long double _value = (value);                                 \
    if (!(_expr == _value)) {                                           \
        FAIL("%s was %Lg but should have been %Lg", #expr, _expr, _value); \
    }                                                                   \
} while (0)

/*-----------------------------------------------------------------------*/

/**
 * EXPECT_PTREQ:  Check that the given pointer expression is equal to (has
 * the same address as) an expected value, and fail the test if not.
 */
#define EXPECT_PTREQ(expr, value)  do {                                 \
    const void * const _expr = (expr);                                  \
    const void * const _value = (value);                                \
    if (_expr != _value) {                                              \
        FAIL("%s was %p but should have been %p", #expr, _expr, _value); \
    }                                                                   \
} while (0)

/*-----------------------------------------------------------------------*/

/* EXPECT_STREQ helper to print line-by-line differences between strings.
 * Defined in common.c. */
extern void _diff_strings(FILE *f, const char *from, const char *to);

/**
 * EXPECT_STREQ:  Check that the given string expression is equal to an
 * expected value, and fail the test if not.  A value of NULL is permitted.
 */
#define EXPECT_STREQ(expr, value)  do {                                 \
    const char * const _expr = (expr);                                  \
    const char * const _value = (value);                                \
    if (_expr && !_value) {                                             \
        FAIL("%s was \"%s\" but should have been NULL", #expr, _expr);  \
    } else if (!_expr && _value) {                                      \
        FAIL("%s was NULL but should have been \"%s\"", #expr, _value); \
    } else if (_expr && _value && strcmp(_expr, _value) != 0) {         \
        if (strchr(_value, '\n')) {                                     \
            fprintf(stderr, "%s:%d: %s differed from the expected text:\n", \
                    __FILE__, __LINE__, #expr);                         \
            _diff_strings(stderr, _value, _expr);                       \
            return EXIT_FAILURE;                                        \
        } else {                                                        \
            FAIL("%s was \"%s\" but should have been \"%s\"",           \
                 #expr, _expr, _value);                                 \
        }                                                               \
    }                                                                   \
} while (0)

/*-----------------------------------------------------------------------*/

/* EXPECT_MEMEQ helper to print differences between memory buffers.
 * Defined in common.c. */
extern void _diff_mem(FILE *f, const uint8_t *from, const uint8_t *to,
                      long len);

/**
 * EXPECT_MEMEQ:  Check that the given memory buffer has the expected
 * contents, and fail the test if not.  A value of NULL is permitted.
 */
#define EXPECT_MEMEQ(buf, expected, len)  do {                          \
    const void * const _buf = (buf);                                    \
    const void * const _expected = (expected);                          \
    const long _len = (len);                                            \
    ASSERT(_len >= 0);                                                  \
    if (_buf && !_expected) {                                           \
        FAIL("%s was %p but should have been NULL", #buf, _buf);        \
    } else if (!_buf && _expected) {                                    \
        FAIL("%s was NULL but should not have been", #buf);             \
    } else if (_buf && _expected && memcmp(_buf, _expected, _len) != 0) { \
        fprintf(stderr, "%s:%d: %s differed from the expected data:\n", \
                    __FILE__, __LINE__, #buf);                          \
        _diff_mem(stderr, _expected, _buf, _len);                       \
        return EXIT_FAILURE;                                            \
    }                                                                   \
} while (0)

/*************************************************************************/
/*************************************************************************/

#endif  // TESTS_COMMON_H
