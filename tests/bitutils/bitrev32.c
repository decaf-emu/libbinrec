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
    EXPECT_EQ(bitrev32(0x11235813), 0xC81AC488);

    return EXIT_SUCCESS;
}
