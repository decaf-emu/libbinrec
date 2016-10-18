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


static struct {
    uint32_t address;
    uint32_t r3;
    uint32_t r4;
} saved_state[4];
static int save_counter;

static void pre_insn_callback(void *state_, uint32_t address)
{
    PPCState *state = state_;
    ASSERT(state);
    ASSERT(save_counter < lenof(saved_state));

    saved_state[save_counter].address = address;
    saved_state[save_counter].r3 = state->gpr[3];
    saved_state[save_counter].r4 = state->gpr[4];
    save_counter++;
}

static void configure_handle(binrec_t *handle)
{
    binrec_set_pre_insn_callback(handle, pre_insn_callback);
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
        0x3880000A,  // li r4,10
        0x38600002,  // li r3,2
        0x4E800020,  // blr
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));
    save_counter = 0;

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         configure_handle, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }
    EXPECT_STREQ(get_log_messages(), NULL);

    EXPECT_EQ(save_counter, 4);
    EXPECT_EQ(saved_state[0].address, 0x1000);
    EXPECT_EQ(saved_state[0].r3, 0);
    EXPECT_EQ(saved_state[0].r4, 0);
    EXPECT_EQ(saved_state[1].address, 0x1004);
    EXPECT_EQ(saved_state[1].r3, 1);
    EXPECT_EQ(saved_state[1].r4, 0);
    EXPECT_EQ(saved_state[2].address, 0x1008);
    EXPECT_EQ(saved_state[2].r3, 1);
    EXPECT_EQ(saved_state[2].r4, 10);
    EXPECT_EQ(saved_state[3].address, 0x100C);
    EXPECT_EQ(saved_state[3].r3, 2);
    EXPECT_EQ(saved_state[3].r4, 10);

    free(memory);
    return EXIT_SUCCESS;
}
