/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include "tests/common.h"


int main(void)
{
    const unsigned int features = binrec_native_features();

    #if defined(__amd64__) || defined(__x86_64__) || defined(_M_X64)
        /* Just copying the source of the function here wouldn't make for
         * a particularly meaningful test, so we assume that at least one
         * feature bit will be set (as will be true on all but the oldest
         * x86-64 processors) and check for non-zeroness. */
        EXPECT(features != 0);
    #else
        EXPECT_EQ(features, 0);
    #endif

    return EXIT_SUCCESS;
}
