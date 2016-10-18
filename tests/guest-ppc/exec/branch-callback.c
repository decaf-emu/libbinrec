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


static int branch_counter;

static int branch_callback(PPCState *state, uint32_t address)
{
    ASSERT(state);

    if (address == 0x1000) {
        return 1;  // Initial branch.
    } else if (address != 0x100C) {
        printf("%s:%d: address was 0x%X but should have been 0x100C\n",
               __FILE__, __LINE__, address);
        branch_counter = -1;
        state->nia = 0x1010;
        return 0;
    } else if (state->nia != 0x100C) {
        printf("%s:%d: state->nia was 0x%X but should have been 0x100C\n",
               __FILE__, __LINE__, state->nia);
        branch_counter = 99;
        state->nia = 0x1010;
        return 0;
    } else {
        ASSERT(branch_counter >= 0 && branch_counter < 3);  // Else we're probably infinite-looping.
        branch_counter++;
        state->nia = 0x1010;  // Should only take effect when breaking out.
        return branch_counter < 3;
    }
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
        0x4800000C,  // b 0x100C
        0x60000000,  // nop
        0x48000000,  // b 0x1008
        0x48000000,  // b 0x100C
        0x4E800020,  // blr
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));
    state.branch_callback = branch_callback;
    branch_counter = 0;

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         NULL, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }
    EXPECT_STREQ(get_log_messages(), NULL);

    EXPECT_EQ(branch_counter, 3);

    free(memory);
    return EXIT_SUCCESS;
}
