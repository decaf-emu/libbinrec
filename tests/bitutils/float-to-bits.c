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
    EXPECT_EQ(float_to_bits(0.0f), 0);
    EXPECT_EQ(float_to_bits(1.0f), 0x3F800000);
    EXPECT_EQ(float_to_bits(-1.0f), 0xBF800000);
    volatile float x = 0.0f;
    EXPECT_EQ(float_to_bits(1.0f / x), 0x7F800000);
    EXPECT_EQ(float_to_bits(0.0f / x) & 0x7FC00000, 0x7FC00000);

    return EXIT_SUCCESS;
}
