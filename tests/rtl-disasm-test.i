/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

/*
 * This source file implements a test which passes a buffer containing
 * guest machine code to an input translator and compares the disassembly
 * of the resulting RTL unit to an expected output string.  It is intended
 * to be #included into a source file which defines the parameters of the test.
 *
 * The test source file should define the following variables before
 * including this file:
 *
 * - static const binrec_setup_t setup
 *      Define this to a setup structure which will be passed to
 *      binrec_create_handle().
 *
 * - static const uint8_t input[]
 *      Define this to an array containing the machine code to translate.
 *
 * - static unsigned int common_opt
 *      Define this to the set of common optimization flags to use.
 *
 * - static unsigned int guest_opt
 *      Define this to the set of guest optimization flags to use.
 *
 * - static const bool expected_success
 *      Define this to the expected return value from the translation
 *      function.
 *
 * - static const char expected[]
 *      Define this to a string containing the expected log messages (if
 *      any) followed by the expected RTL disassembly (if disassembly was
 *      successful).
 *
 * If the preprocessor symbol CHAINING is defined, chaining will be enabled.
 *
 * If the preprocessor symbol BRANCH_CALLBACK is defined, the branch
 * callback will be enabled.
 *
 * If the preprocessor symbols PRE_INSN_CALLBACK or POST_INSN_CALLBACK are
 * defined, they will be passed to binrec_set_pre_insn_callback() or
 * binrec_set_post_insn_callback() respectively.
 *
 * If the preprocessor symbol INITIAL_STATE is defined to a state block
 * pointer, that pointer will be stored in handle->opt_state before the
 * guest translator is called.
 */

#include "src/common.h"
#include "src/guest-ppc.h"
#include "src/rtl.h"
#include "tests/common.h"
#include "tests/log-capture.h"


int main(void)
{
    void *aligned_input;
    ASSERT(aligned_input = malloc(sizeof(input)));
    memcpy(aligned_input, input, sizeof(input));

    binrec_setup_t final_setup = setup;
    final_setup.guest_memory_base = aligned_input;
    final_setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&final_setup));

    binrec_set_optimization_flags(handle, common_opt, guest_opt, 0);
    binrec_set_code_range(handle, 0, sizeof(input) - 1);
    #ifdef CHAINING
        binrec_enable_chaining(handle, true);
    #endif
    #ifdef BRANCH_CALLBACK
        binrec_enable_branch_callback(handle, true);
    #endif
    #ifdef PRE_INSN_CALLBACK
        binrec_set_pre_insn_callback(handle, PRE_INSN_CALLBACK);
    #endif
    #ifdef POST_INSN_CALLBACK
        binrec_set_post_insn_callback(handle, POST_INSN_CALLBACK);
    #endif
    #ifdef INITIAL_STATE
        handle->opt_state = INITIAL_STATE;
    #endif

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    if (guest_ppc_translate(handle, 0, sizeof(input) - 1,
                            unit) != expected_success) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("guest_ppc_translate(handle, 0, 0x%X, unit) did not %s as"
             " expected", (int)sizeof(input) - 1,
             expected_success ? "succeed" : "fail");
    }

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly;
    if (expected_success) {
        EXPECT(disassembly = rtl_disassemble_unit(unit, false));
    } else {
        disassembly = "";
    }

    char *output;
    const char *log_messages = get_log_messages();
    if (!log_messages) {
        log_messages = "";
    }

    const int output_size = strlen(log_messages) + strlen(disassembly) + 1;
    ASSERT(output = malloc(output_size));
    ASSERT(snprintf(output, output_size, "%s%s", log_messages, disassembly)
           == output_size - 1);
    EXPECT_STREQ(output, expected);

    free(output);
    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    free(aligned_input);
    return EXIT_SUCCESS;
}
