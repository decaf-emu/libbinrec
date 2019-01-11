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
#include "tests/execute.h"
#include "tests/guest-ppc/common.h"
#include "tests/log-capture.h"


int main(void)
{
    if (!binrec_host_supported(binrec_native_arch())) {
        printf("Skipping test because native architecture not supported.\n");
        return EXIT_SUCCESS;
    }

    uint8_t *memory;
    EXPECT(memory = malloc(0x10000));

    static const uint32_t ppc_code[] = {
        0xE0230000,  // psq_l f1,0(r3),0,0
        0xE0430008,  // psq_l f2,8(r3),0,0
        0xF0230010,  // psq_st f1,16(r3),0,0
        0xF0430018,  // psq_st f1,24(r3),0,0
        0x4E800020,  // blr
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));
    state.gpr[3] = 0x2000;
    *(uint32_t *)(memory+0x2000) = bswap_be32(0x00000001);
    *(uint32_t *)(memory+0x2004) = bswap_be32(0x80000002);
    *(uint32_t *)(memory+0x2008) = bswap_be32(0x80000003);
    *(uint32_t *)(memory+0x200C) = bswap_be32(0x00000004);

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         NULL, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }

    EXPECT_EQ(bswap_be32(*(uint32_t *)(memory+0x2010)), 0x00000000);
    EXPECT_EQ(bswap_be32(*(uint32_t *)(memory+0x2014)), 0x80000000);
    EXPECT_EQ(bswap_be32(*(uint32_t *)(memory+0x2018)), 0x80000000);
    EXPECT_EQ(bswap_be32(*(uint32_t *)(memory+0x201C)), 0x00000000);

    free(memory);
    return EXIT_SUCCESS;
}
