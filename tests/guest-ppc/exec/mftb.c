/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include "src/endian.h"
#include "tests/common.h"
#include "tests/execute.h"
#include "tests/guest-ppc/common.h"
#include "tests/log-capture.h"


static uint64_t tb;  // 64-bit timebase value to return; incremented each call.

static uint64_t timebase_handler(PPCState *state)
{
    ASSERT(state);
    return tb++;
}


int main(void)
{
    if (!binrec_host_supported(binrec_native_arch())) {
        printf("Skipping test because native architecture not supported.\n");
        return EXIT_SUCCESS;
    }

    uint8_t *memory;
    EXPECT(memory = malloc(0x10000));

    static const uint32_t ppc_code[] = {
        0x7C6C42E6,  // mftb r3
        0x7C8D42E6,  // mftbu r4
        0x7CAC42E6,  // mftb r5
        0x7CCD42E6,  // mftbu r6
        0x4E800020,  // blr
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));
    state.timebase_handler = timebase_handler;
    tb = UINT64_C(0x8FFFFFFFD);

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         NULL, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }

    EXPECT_EQ(state.gpr[3], 0xFFFFFFFD);
    EXPECT_EQ(state.gpr[4], 8);
    EXPECT_EQ(state.gpr[5], 0xFFFFFFFF);
    EXPECT_EQ(state.gpr[6], 9);
    EXPECT_EQ(tb, UINT64_C(0x900000001));

    free(memory);
    return EXIT_SUCCESS;
}
