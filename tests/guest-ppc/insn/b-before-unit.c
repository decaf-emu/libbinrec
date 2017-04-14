/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

static const uint8_t input[] = {
    0x60,0x00,0x00,0x00,  // nop
    0x4B,0xFF,0xF0,0x00,  // b 0x4
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 4\n"
    "    3: SET_ALIAS  a1, r3\n"
    "    4: GOTO       L1\n"
    "    5: LOAD_IMM   r4, 4104\n"
    "    6: SET_ALIAS  a1, r4\n"
    "    7: LABEL      L1\n"
    "    8: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 2\n"
    "Block 1: <none> --> [5,6] --> 2\n"
    "Block 2: 1,0 --> [7,8] --> <none>\n"
    ;

/* Tweaked version of tests/rtl-disasm-test.i to start at a nonzero address. */

#include "src/common.h"
#include "src/guest-ppc.h"
#include "src/rtl.h"
#include "tests/common.h"
#include "tests/log-capture.h"


int main(void)
{
    const uint32_t start = 0x1000;

    void *aligned_input;
    ASSERT(aligned_input = malloc(start + sizeof(input)));
    memcpy((uint8_t *)aligned_input + start, input, sizeof(input));

    binrec_setup_t final_setup = setup;
    final_setup.guest_memory_base = aligned_input;
    final_setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&final_setup));

    binrec_set_optimization_flags(handle, 0, guest_opt, 0);
    binrec_set_code_range(handle, start, start + sizeof(input) - 1);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    if (guest_ppc_translate(handle, start, start + sizeof(input) - 1,
                            unit) != expected_success) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("guest_ppc_translate(handle, 0x%X, 0x%X, unit) did not %s as"
             " expected", start, start + (int)sizeof(input) - 1,
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
