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

static const FailureRecord expected_error_list[] = {
    EXPECTED_ERRORS_COMMON,
};


static void configure_handle(binrec_t *handle)
{
    binrec_enable_branch_exit_test(handle, true);
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
    state.branch_exit_flag = true;  // Force an exit at every branch.

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

    const bool success = check_750cl_errors(
        state.gpr[3], memory, expected_error_list, lenof(expected_error_list));

    free(memory);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
