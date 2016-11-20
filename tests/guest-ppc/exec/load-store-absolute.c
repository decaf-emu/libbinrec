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
    memset(memory, -1, 0x10000);
    memset(&memory[0x400], 0, 0x10);
    memset(&memory[0x4000], 0, 0x20);

    static const uint32_t ppc_code[] = {
        0x90604000,  // stw r3,0x4000(0)
        0xB0604006,  // sth r3,0x4006(0)
        0x98604005,  // stb r3,0x4005(0)
        0xD8204008,  // stfd f1,0x4008(0)
        0xD0204010,  // stfs f1,0x4010(0)
        0xBFA04014,  // stmw r29,0x4014(0)
        0x80804000,  // lwz r4,0x4000(0)
        0xA0A04004,  // lhz r5,0x4004(0)
        0x88C04006,  // lbz r6,0x4006(0)
        0xC8404008,  // lfd f2,0x4008(0)
        0xC0604010,  // lfs f3,0x4010(0)
        0xBBA04000,  // lmw r29,0x4000(0)
        0xD8200400,  // stfd f1,0x400(0)
        0xE0800400,  // psq_l f4,0x400(0)
        0xF0800408,  // psq_st f4,0x408(0)
        0x4E800020,  // blr
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));
    state.gpr[0] = 0x2000;
    state.gpr[3] = 0x12345678;
    state.gpr[29] = 290;
    state.gpr[30] = 300;
    state.gpr[31] = 310;
    state.fpr[1][0] = 1.0;

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         log_capture, NULL, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }
    EXPECT_EQ(state.gpr[3], 0x12345678);
    EXPECT_EQ(state.gpr[4], 0x12345678);
    EXPECT_EQ(state.gpr[5], 0x78);
    EXPECT_EQ(state.gpr[6], 0x56);
    EXPECT_EQ(state.gpr[29], 0x12345678);
    EXPECT_EQ(state.gpr[30], 0x785678);
    EXPECT_EQ(state.gpr[31], 0x3FF00000);
    EXPECT_FLTEQ(state.fpr[2][0], 1.0);
    EXPECT_FLTEQ(state.fpr[2][1], 0.0);
    EXPECT_FLTEQ(state.fpr[3][0], 1.0);
    EXPECT_FLTEQ(state.fpr[3][1], 1.0);
    EXPECT_FLTEQ(state.fpr[4][0], 1.875);
    EXPECT_FLTEQ(state.fpr[4][1], 0.0);
    EXPECT_MEMEQ(&memory[0x4000], ("\x12\x34\x56\x78\x00\x78\x56\x78"
                                   "\x3F\xF0\x00\x00\x00\x00\x00\x00"
                                   "\x3F\x80\x00\x00\x00\x00\x01\x22"
                                   "\x00\x00\x01\x2C\x00\x00\x01\x36"), 32);
    EXPECT_MEMEQ(&memory[0x400], ("\x3F\xF0\x00\x00\x00\x00\x00\x00"
                                  "\x3F\xF0\x00\x00\x00\x00\x00\x00"), 16);

    free(memory);
    return EXIT_SUCCESS;
}
