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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************/
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
        FAIL("%s was not true as expected", #expr);                     \
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
        FAIL("%s was \"%s\" but should have been \"%s\"",               \
             #expr, _expr, _value);                                     \
    }                                                                   \
} while (0)

/*************************************************************************/
/*************************************************************************/

#endif  // TESTS_COMMON_H
