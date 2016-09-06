/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/bitutils.h"
#include "tests/common.h"
#include <inttypes.h>


int main(int argc, char **argv)
{
    for (int i = 0; i <= 64; i++) {
        uint64_t x = 0;
        for (int j = 0; j < i; j++) {
            x |= UINT64_C(1) << ((j * 5) % 64);
        }
        const int result = popcnt64(x);
        if (result != i) {
            FAIL("popcnt64(0x%"PRIX64") was %d but should have been %d",
                 x, result, i);
        }
    }

    return EXIT_SUCCESS;
}
