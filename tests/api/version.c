/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include "tests/common.h"

#ifndef VERSION
# error VERSION must be defined to the expected version string.
#endif


int main(void)
{
    const char *version = binrec_version();
    EXPECT_STREQ(version, VERSION);
    return EXIT_SUCCESS;
}
