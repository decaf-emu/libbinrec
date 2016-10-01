/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#define SUPPRESS_ENDIAN_INTRINSICS
#include "src/endian.h"
#include "tests/common.h"


int main(int argc, char **argv)
{
    EXPECT_EQ(bswap16(0x1234), 0x3412);

    volatile uint16_t test = 0x1234;
    EXPECT_EQ(bswap16(test), 0x3412);

    return EXIT_SUCCESS;
}
