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
    const binrec_arch_t arch = binrec_native_arch();

    #if defined(__amd64__) || defined(__x86_64__) || defined(_M_X64)
        #ifdef _WIN32
            EXPECT_EQ(arch, BINREC_ARCH_X86_64_WINDOWS);
        #else
            EXPECT_EQ(arch, BINREC_ARCH_X86_64_SYSV);
        #endif
    #else
        EXPECT_EQ(arch, BINREC_ARCH_INVALID);
    #endif

    return EXIT_SUCCESS;
}
