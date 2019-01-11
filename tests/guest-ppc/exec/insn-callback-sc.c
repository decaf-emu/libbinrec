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


static int pre_insn_count;
static int post_insn_count;

static void pre_insn_callback(UNUSED void *state, UNUSED uint32_t address)
{
    pre_insn_count++;
}

static void post_insn_callback(UNUSED void *state, UNUSED uint32_t address)
{
    post_insn_count++;
}

static PPCState *sc_handler(PPCState *state, UNUSED uint32_t insn)
{
    return state;
}

static void configure_handle(binrec_t *handle)
{
    binrec_set_pre_insn_callback(handle, pre_insn_callback);
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
        0x44000002,  // sc
        0x4E800020,  // blr
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));
    state.sc_handler = sc_handler;
    pre_insn_count = 0;
    post_insn_count = 0;

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         configure_handle, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }

    EXPECT_EQ(pre_insn_count, 2);
    EXPECT_EQ(post_insn_count, 2);

    free(memory);
    return EXIT_SUCCESS;
}
