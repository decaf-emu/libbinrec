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
#include "tests/log-capture.h"


int main(void)
{
    if (!binrec_host_supported(binrec_native_arch())) {
        printf("Skipping test because native architecture not supported.\n");
        return EXIT_SUCCESS;
    }

    uint8_t *memory;
    EXPECT(memory = malloc(0x2000000));

    static const uint8_t ppc_code[] = {
        0x38,0x60,0x00,0x01,  // li r3,1
        0x38,0x80,0x00,0x0A,  // li r4,10
    };
    const uint32_t start_address = 0x1000;
    memcpy(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stderr);
        }
        FAIL("Failed to execute guest code");
    }
    EXPECT_STREQ(get_log_messages(), "[info] Scanning terminated at requested"
                 " limit 0x1007\n");

    EXPECT_EQ(state.gpr[3], 1);
    EXPECT_EQ(state.gpr[4], 10);

    return EXIT_SUCCESS;
}
