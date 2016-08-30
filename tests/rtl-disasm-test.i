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
 * - static const uint8_t input[]
 *      Define this to an array containing the machine code to translate.
 *
 * - static const bool expected_success
 *      Define this to the expected return value from the translation
 *      function.
 *
 * - static const char expected[]
 *      Define this to a string containing the expected log messages (if
 *      any) followed by the expected RTL disassembly (if disassembly was
 *      successful).
 */

#include "src/common.h"
#include "src/guest-ppc.h"
#include "src/rtl.h"
#include "tests/common.h"

static char *log_messages = NULL;

static void do_log(UNUSED void *userdata, binrec_loglevel_t loglevel,
                   const char *message)
{
    static const char * const level_prefix[] = {
        [BINREC_LOGLEVEL_INFO   ] = "info",
        [BINREC_LOGLEVEL_WARNING] = "warning",
        [BINREC_LOGLEVEL_ERROR  ] = "error",
    };
    const int current_len = log_messages ? strlen(log_messages) : 0;
    const int message_len =
        snprintf(NULL, 0, "[%s] %s\n", level_prefix[loglevel], message);
    const int new_size = current_len + message_len + 1;
    ASSERT(log_messages = realloc(log_messages, new_size));
    ASSERT(snprintf(log_messages + current_len, message_len + 1, "[%s] %s\n",
                    level_prefix[loglevel], message) == message_len);
}

int main(int argc, char **argv)
{
    void *aligned_input;
    ASSERT(aligned_input = malloc(sizeof(input)));
    memcpy(aligned_input, input, sizeof(input));

    binrec_setup_t final_setup = setup;
    final_setup.memory_base = aligned_input;
    final_setup.log = do_log;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&final_setup));

    binrec_set_code_range(handle, 0, sizeof(input) - 1);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    if (guest_ppc_translate(handle, 0, unit) != expected_success) {
        if (log_messages) {
            fputs(log_messages, stderr);
        }
        FAIL("guest_ppc_translate(handle, 0, unit) did not %s as expected",
             expected_success ? "succeed" : "fail");
    }

    char *disassembly;
    if (expected_success) {
        EXPECT(disassembly = rtl_disassemble_unit(unit, false));
    } else {
        ASSERT(disassembly = malloc(1));
        *disassembly = '\0';
    }

    char *output;
    if (log_messages) {
        const int output_size = strlen(log_messages) + strlen(disassembly) + 1;
        ASSERT(output = malloc(output_size));
        ASSERT(snprintf(output, output_size, "%s%s", log_messages, disassembly)
               == output_size - 1);
        free(disassembly);
    } else {
        output = disassembly;
    }
    EXPECT_STREQ(output, expected);

    free(output);
    binrec_destroy_handle(handle);
    free(aligned_input);
    return EXIT_SUCCESS;
}
