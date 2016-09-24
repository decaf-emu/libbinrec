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
#include <math.h>


int main(int argc, char **argv)
{
    EXPECT_EQ(double_to_bits(0.0), 0);
    EXPECT_EQ(double_to_bits(1.0), UINT64_C(0x3FF0000000000000));
    EXPECT_EQ(double_to_bits(-1.0), UINT64_C(0xBFF0000000000000));
    volatile double x = 0.0;
    EXPECT_EQ(double_to_bits(1.0 / x), UINT64_C(0x7FF0000000000000));
    EXPECT_EQ(double_to_bits(0.0 / x) & UINT64_C(0x7FF8000000000000),
              UINT64_C(0x7FF8000000000000));

    return EXIT_SUCCESS;
}
