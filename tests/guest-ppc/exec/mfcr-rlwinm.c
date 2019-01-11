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
        0x2C03000A,  // cmpwi r3,10
        0x7C800026,  // mfcr r4
        0x5484273E,  // rlwinm r4,r4,4,28,31
        0x7CA00026,  // mfcr r5
        0x54A50FFE,  // rlwinm. r5,r5,1,31,31
        0x7CC00026,  // mfcr r6
        0x54C617FE,  // rlwinm r6,r6,2,31,31
        0x7CE00026,  // mfcr r7
        0x54E71FFF,  // rlwinm. r7,r7,3,31,31
        0x4182000C,  // beq .+12
        0x39000001,  // li r8,1
        0x4E800020,  // blr
        0x39000000,  // li r8,0
        0x4E800020,  // blr
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));

    state.gpr[3] = 5;
    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         NULL, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }
    EXPECT_EQ(state.gpr[3], 5);
    EXPECT_EQ(state.gpr[4], 8);
    EXPECT_EQ(state.gpr[5], 1);
    EXPECT_EQ(state.gpr[6], 0);
    EXPECT_EQ(state.gpr[7], 0);
    EXPECT_EQ(state.gpr[8], 0);

    state.gpr[3] = 15;
    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         NULL, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }
    EXPECT_EQ(state.gpr[3], 15);
    EXPECT_EQ(state.gpr[4], 4);
    EXPECT_EQ(state.gpr[5], 0);
    EXPECT_EQ(state.gpr[6], 1);
    EXPECT_EQ(state.gpr[7], 0);
    EXPECT_EQ(state.gpr[8], 0);

    state.gpr[3] = 10;
    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         NULL, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }
    EXPECT_EQ(state.gpr[3], 10);
    EXPECT_EQ(state.gpr[4], 2);
    EXPECT_EQ(state.gpr[5], 0);
    EXPECT_EQ(state.gpr[6], 0);
    EXPECT_EQ(state.gpr[7], 1);
    EXPECT_EQ(state.gpr[8], 1);

    free(memory);
    return EXIT_SUCCESS;
}
