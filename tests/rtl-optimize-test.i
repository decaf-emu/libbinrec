/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

/*
 * This source file implements a test which performs selected optimizations
 * on an RTL instruction sequence and compares the disassembly of the
 * resulting RTL unit to an expected output string.  It is intended to be
 * #included into a source file which defines the parameters of the test.
 *
 * The test source file should define the following variables before
 * including this file:
 *
 * - static const unsigned int opt_flags
 *      Define this to the set of RTL optimization flags to enable.
 *
 * - bool add_rtl(RTLUnit *unit)
 *      This function will be called to fill the RTL unit with
 *      instructions before calling the translation function.  It should
 *      return EXIT_SUCCESS on success, EXIT_FAILURE if any errors occur.
 *
 * - static const char expected[]
 *      Define this to a string containing the expected log messages (if
 *      any) followed by the expected RTL disassembly.
 */

#include "src/common.h"
#include "src/rtl.h"
#include "tests/common.h"
#include "tests/log-capture.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));
    binrec_set_optimization_flags(handle, opt_flags, 0, 0);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    if (add_rtl(unit) != EXIT_SUCCESS) {
        const int line = __LINE__ - 1;
        const char *log_messages = get_log_messages();
        printf("%s:%d: add_rtl(unit) failed (%s)\n%s", __FILE__, line,
               log_messages ? "log follows" : "no errors logged",
               log_messages ? log_messages : "");
        return EXIT_FAILURE;
    }

    if (!rtl_finalize_unit(unit)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("rtl_finalize_unit(unit) was not true as expected");
    }

    if (!rtl_optimize_unit(unit, opt_flags)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("rtl_optimize_unit(unit, opt_flags) was not true as expected");
    }

    const char *disassembly;
    EXPECT(disassembly = rtl_disassemble_unit(unit, false));

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
    return EXIT_SUCCESS;
}
