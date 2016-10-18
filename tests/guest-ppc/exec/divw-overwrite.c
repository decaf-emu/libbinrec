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


/* Check for a former bug in which the source aliases were not properly
 * flushed before the test for divide overflow, causing the division
 * result to be lost. */

int main(void)
{
    if (!binrec_host_supported(binrec_native_arch())) {
        printf("Skipping test because native architecture not supported.\n");
        return EXIT_SUCCESS;
    }

    uint8_t *memory;
    EXPECT(memory = malloc(0x10000));

    static const uint32_t ppc_code[] = {
        0x38600007,  // li r3,7
        0x38800003,  // li r4,3
        0x7C8323D6,  // divw r4,r3,r4
        0x7C832378,  // mr r3,r4
        0x4E800020,  // blr
        0x38600007,  // li r3,7
        0x38800003,  // li r4,3
        0x7C6323D6,  // divw r3,r3,r4
        0x7C641B78,  // mr r4,r3
        0x4E800020,  // blr
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         NULL, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }
    EXPECT_STREQ(get_log_messages(), NULL);
    EXPECT_EQ(state.gpr[3], 2);
    EXPECT_EQ(state.gpr[4], 2);

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory,
                         start_address + 20, NULL, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }
    EXPECT_STREQ(get_log_messages(), NULL);
    EXPECT_EQ(state.gpr[3], 2);
    EXPECT_EQ(state.gpr[4], 2);

    free(memory);
    return EXIT_SUCCESS;
}
