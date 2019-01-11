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


static int callback_counter = 0;

static void post_insn_callback(void *state_, uint32_t address)
{
    ASSERT(state_);
    PPCState *state = state_;

    callback_counter++;
    if (callback_counter == 4) {
        /* Set a value with the low byte clear to verify that the code
         * reads the full 32-bit value. */
        state->branch_exit_flag = 1<<31;
        /* Modify r3 so the code exits. */
        state->gpr[3] = 1;
    }
}

static void configure_handle(binrec_t *handle)
{
    binrec_enable_branch_exit_test(handle, true);

    /* We use the post-instruction callback to set the exit flag during
     * execution without needing a separate thread. */
    binrec_set_post_insn_callback(handle, post_insn_callback);
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
        0x2C030000,  // cmpwi r3,0
        0x4182FFFC,  // beq 0x1000
        0x4E800020,  // blr
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));

    callback_counter = 0;
    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         configure_handle, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }

    EXPECT_EQ(callback_counter, 7);

    free(memory);
    return EXIT_SUCCESS;
}
