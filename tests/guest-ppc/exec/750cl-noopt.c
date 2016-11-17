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
                         PPC750CL_START_ADDRESS, log_capture, NULL, NULL)) {
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
    const int expected_errors = 5;
    const int result = state.gpr[3];
    if (result < 0) {
        exitcode = EXIT_FAILURE;
        printf("Test failed to bootstrap (error %d)\n", result);
    } else if (result != expected_errors) {
        exitcode = EXIT_FAILURE;
        printf("Wrong number of errors returned (expected %d, got %d)\n",
               expected_errors, result);
        printf("Error log follows:\n");
        print_750cl_errors(result, memory);
    }
    // FIXME: eventually record all expected errors to make sure they match
    // - 7C60212D 0100A5E0 (stwcx. to different address)
    // - C89F0008 0100B778 (lfd/ps data hazard)
    // - 10652C20 0100B81C (ps_* read double-precision denormal as 00800000)
    // - 11A01834 01011990 (ps_rsqrte on huge double-precision value)
    // - 11A02034 01011A24 (ps_rsqrte on tiny double-precision value)

    free(memory);
    return exitcode;
}
