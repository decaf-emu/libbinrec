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
#include <inttypes.h>


int main(int argc, char **argv)
{
    for (int i = 0; i < 64; i++) {
        const uint64_t x = UINT64_C(1) << i;
        const int result = clz64(x);
        if (result != 63-i) {
            FAIL("clz64(0x%"PRIX64") was %d but should have been %d",
                 x, result, 63-i);
        }
    }

    EXPECT_EQ(clz64(0), 64);

    return EXIT_SUCCESS;
}
