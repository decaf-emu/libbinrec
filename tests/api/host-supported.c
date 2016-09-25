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


int main(void)
{
    EXPECT_FALSE(binrec_host_supported(BINREC_ARCH_INVALID));
    EXPECT_FALSE(binrec_host_supported(BINREC_ARCH_PPC_7XX));
    EXPECT(binrec_host_supported(BINREC_ARCH_X86_64_SYSV));
    EXPECT(binrec_host_supported(BINREC_ARCH_X86_64_WINDOWS));
    EXPECT(binrec_host_supported(BINREC_ARCH_X86_64_WINDOWS_SEH));

    return EXIT_SUCCESS;
}
