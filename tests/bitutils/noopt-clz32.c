/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#define SUPPRESS_BITUTILS_INTRINSICS
#include "src/bitutils.h"
#include "tests/common.h"


int main(int argc, char **argv)
{
    for (int i = 0; i < 32; i++) {
        const uint32_t x = UINT32_C(1) << i;
        const int result = clz32(x);
        if (result != 31-i) {
            FAIL("clz32(0x%X) was %d but should have been %d",
                 x, result, 31-i);
        }
    }

    EXPECT_EQ(clz32(0), 32);

    return EXIT_SUCCESS;
}
