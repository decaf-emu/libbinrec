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
    const unsigned int common_opt = BINREC_OPT_BASIC
                                  | BINREC_OPT_DSE
                                  | BINREC_OPT_DECONDITION
                                  | BINREC_OPT_DEEP_DATA_FLOW
                                  | BINREC_OPT_FOLD_CONSTANTS;
    ASSERT(binrec_native_arch() == BINREC_ARCH_X86_64_SYSV
        || binrec_native_arch() == BINREC_ARCH_X86_64_WINDOWS);
    const unsigned int host_opt = BINREC_OPT_H_X86_ADDRESS_OPERANDS
                                | BINREC_OPT_H_X86_BRANCH_ALIGNMENT
                                | BINREC_OPT_H_X86_CONDITION_CODES
                                | BINREC_OPT_H_X86_FIXED_REGS
                                | BINREC_OPT_H_X86_FORWARD_CONDITIONS
                                | BINREC_OPT_H_X86_MERGE_REGS
                                | BINREC_OPT_H_X86_STORE_IMMEDIATE;
    binrec_set_optimization_flags(handle, common_opt, 0, host_opt);
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

    const bool success = check_750cl_errors(
        state.gpr[3], memory, expected_error_list, lenof(expected_error_list));

    free(memory);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
