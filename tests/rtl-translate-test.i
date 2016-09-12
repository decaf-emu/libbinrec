/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

/*
 * This source file implements a test which passes an instruction sequence
 * to an input translator and compares the disassembly of the resulting RTL
 * unit to an expected output string.  It is intended to be #included into
 * a source file which defines the parameters of the test.
 *
 * The test source file should define the following variables before
 * including this file:
 *
 * - static const binrec_setup_t setup
 *      Define this to a setup structure which will be passed to
 *      binrec_create_handle().
 *
 * - static const unsigned int host_opt
 *      Define this to the set of host optimization flags to enable.
 *
 * - bool add_rtl(RTLUnit *unit)
 *      This function will be called to fill the RTL unit with
 *      instructions before calling the translation function.  It should
 *      return EXIT_SUCCESS on success, EXIT_FAILURE if any errors occur.
 *
 * - static const uint8_t expected_code[]
 *      Define this to a buffer containing the expected translation output.
 *      If empty, the test will expect translation to fail.
 *
 * - static const char expected_log[]
 *      Define this to a buffer containing the expected log messages, if any.
 */

#include "src/common.h"
#include "src/host-x86.h"
#include "src/memory.h"
#include "src/rtl.h"
#include "tests/common.h"
#include "tests/log-capture.h"


int main(void)
{
    binrec_setup_t final_setup = setup;
    final_setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&final_setup));
    binrec_set_optimization_flags(handle, 0, 0, host_opt);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    if (add_rtl(unit) != EXIT_SUCCESS) {
        const int line = __LINE__ - 1;
        const char *log_messages = get_log_messages();
        fprintf(stderr, "%s:%d: add_rtl(unit) failed (%s)\n%s", __FILE__, line,
                log_messages ? "log follows" : "no errors logged",
                log_messages ? log_messages : "");
        return EXIT_FAILURE;
    }

    EXPECT(rtl_finalize_unit(unit));

    handle->code_buffer_size = max(sizeof(expected_code), 1);
    handle->code_alignment = 16;
    EXPECT(handle->code_buffer = binrec_code_malloc(
               handle, handle->code_buffer_size, handle->code_alignment));

    if (sizeof(expected_code) > 0) {
        EXPECT(host_x86_translate(handle, unit));
        EXPECT_MEMEQ(handle->code_buffer, expected_code,
                     sizeof(expected_code));
        EXPECT_EQ(handle->code_len, sizeof(expected_code));
    } else {
        EXPECT_FALSE(host_x86_translate(handle, unit));
    }

    const char *log_messages = get_log_messages();
    EXPECT_STREQ(log_messages, *expected_log ? expected_log : NULL);

    binrec_code_free(handle, handle->code_buffer);
    handle->code_buffer = NULL;
    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
