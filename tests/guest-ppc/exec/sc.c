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
#include "tests/guest-ppc/exec/common.h"
#include "tests/log-capture.h"


static int sc_r3;  // Value of r3 at sc instruction.
static int sc_nia;  // Value of NIA at sc instruction.

static void sc_handler(PPCState *state)
{
    ASSERT(state);

    sc_r3 = state->gpr[3];
    sc_nia = state->nia;
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
        0x38600001,  // li r3,1
        0x44000002,  // sc
        0x38600002,  // li r3,2
        0x4E800020,  // blr
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));
    state.sc_handler = sc_handler;

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         0, 0, 0, 0, 0)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }
    EXPECT_STREQ(get_log_messages(), NULL);

    EXPECT_EQ(state.gpr[3], 2);
    EXPECT_EQ(sc_r3, 1);
    EXPECT_EQ(sc_nia, start_address + 8);

    free(memory);
    return EXIT_SUCCESS;
}
