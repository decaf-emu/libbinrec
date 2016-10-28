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

#include "tests/guest-ppc/exec/750cl-common.i"


static void configure_handle(binrec_t *handle)
{
    binrec_set_optimization_flags(handle,
                                  0, BINREC_OPT_G_PPC_TRIM_CR_STORES, 0);
}

int main(void)
{
    if (!binrec_host_supported(binrec_native_arch())) {
        printf("Skipping test because native architecture not supported.\n");
        return EXIT_SUCCESS;
    }

    PPCState state;
    void *memory;
    EXPECT(memory = setup_750cl(&state));

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory,
                         PPC750CL_START_ADDRESS, log_capture,
                         configure_handle, NULL)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stderr);
        }
        FAIL("Failed to execute guest code");
    }

    const char *log = get_log_messages();
    if (!log) {
        log = "";
    }
    while (*log) {
        if (strncmp(log, "[info]", 6) != 0) {
            break;
        }
        log += strcspn(log, "\n");
        ASSERT(*log == '\n');
        log++;
    }
    EXPECT_STREQ(log, "");

    int exitcode = EXIT_SUCCESS;
    const int expected_errors = 1077;
    const int result = state.gpr[3];
    if (result < 0) {
        exitcode = EXIT_FAILURE;
        printf("Test failed to bootstrap (error %d)\n", result);
    } else if (result > expected_errors) {  // FIXME: temp > instead of != since we might get fewer errors due to uninitialized data
        exitcode = EXIT_FAILURE;
        printf("Wrong number of errors returned (expected %d, got %d)\n",
               expected_errors, result);
        printf("Error log follows:\n");
        const uint32_t *error_log =
            (const uint32_t *)((uintptr_t)memory + PPC750CL_ERROR_LOG_ADDRESS);
        for (int i = 0; i < result; i++, error_log += 8) {
            printf("    %08X %08X  %08X %08X  %08X %08X\n",
                   bswap_be32(error_log[0]), bswap_be32(error_log[1]),
                   bswap_be32(error_log[2]), bswap_be32(error_log[3]),
                   bswap_be32(error_log[4]), bswap_be32(error_log[5]));
        }
    }
    // FIXME: eventually record all expected errors to make sure they match

    free(memory);
    return exitcode;
}