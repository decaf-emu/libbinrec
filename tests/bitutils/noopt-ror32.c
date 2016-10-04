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


int main(int argc, char **argv)
{
    EXPECT_EQ(ror32(0x12345678, 12), 0x67812345);
    EXPECT_EQ(ror32(0x12345678, 40), 0x78123456);
    EXPECT_EQ(ror32(0x12345678, 0), 0x12345678);
    EXPECT_EQ(ror32(0x12345678, 32), 0x12345678);

    return EXIT_SUCCESS;
}
