/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/endian.h"
#include "tests/common.h"


int main(int argc, char **argv)
{
    EXPECT_EQ(bswap64(UINT64_C(0x123456789ABCDEF0)),
              UINT64_C(0xF0DEBC9A78563412));
    return EXIT_SUCCESS;
}
