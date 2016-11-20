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

    static const uint32_t ppc_code[] = {
        0x100327EC,  // dcbz_l r3,r4
        0x4E800020,  // blr
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));
    state.gpr[3] = 0x7FF0;
    state.gpr[4] = 0x21;

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         log_capture, NULL, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }
    EXPECT_MEMEQ(&memory[0x7FF0], ("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                   "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                   "\x00\x00\x00\x00\x00\x00\x00\x00"
                                   "\x00\x00\x00\x00\x00\x00\x00\x00"
                                   "\x00\x00\x00\x00\x00\x00\x00\x00"
                                   "\x00\x00\x00\x00\x00\x00\x00\x00"
                                   "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                   "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"), 64);

    free(memory);
    return EXIT_SUCCESS;
}
