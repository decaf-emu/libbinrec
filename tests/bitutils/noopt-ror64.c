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
    EXPECT_EQ(ror64(UINT64_C(0x123456789ABCDEF0), 12),
              UINT64_C(0xEF0123456789ABCD));
    EXPECT_EQ(ror64(UINT64_C(0x123456789ABCDEF0), 40),
              UINT64_C(0x789ABCDEF0123456));
    EXPECT_EQ(ror64(UINT64_C(0x123456789ABCDEF0), 80),
              UINT64_C(0xDEF0123456789ABC));
    EXPECT_EQ(ror64(UINT64_C(0x123456789ABCDEF0), 0),
              UINT64_C(0x123456789ABCDEF0));
    EXPECT_EQ(ror64(UINT64_C(0x123456789ABCDEF0), 64),
              UINT64_C(0x123456789ABCDEF0));

    return EXIT_SUCCESS;
}
