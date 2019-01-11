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
    0x4F,0xDE,0xF2,0x42,  // crset 30
    0x4F,0xFF,0xF9,0x82,  // crclr 31
    0x48,0x00,0x00,0x04,  // b 0xC
    0x4F,0xDE,0xF1,0x82,  // crclr 30
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_TRIM_CR_STORES
                                    | BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "[warning] No memory for block visited flags (2 bytes), skipping TRIM_CR_STORES optimization\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 1, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: BFEXT      r5, r3, 0, 1\n"
    "    6: SET_ALIAS  a4, r5\n"
    "    7: LOAD_IMM   r6, 1\n"
    "    8: SET_ALIAS  a3, r6\n"
    "    9: LOAD_IMM   r7, 0\n"
    "   10: SET_ALIAS  a4, r7\n"
    "   11: GOTO       L1\n"
    "   12: LOAD_IMM   r8, 12\n"
    "   13: SET_ALIAS  a1, r8\n"
    "   14: LABEL      L1\n"
    "   15: LOAD_IMM   r9, 0\n"
    "   16: SET_ALIAS  a3, r9\n"
    "   17: LOAD_IMM   r10, 16\n"
    "   18: SET_ALIAS  a1, r10\n"
    "   19: GET_ALIAS  r11, a2\n"
    "   20: ANDI       r12, r11, -4\n"
    "   21: GET_ALIAS  r13, a3\n"
    "   22: SLLI       r14, r13, 1\n"
    "   23: OR         r15, r12, r14\n"
    "   24: GET_ALIAS  r16, a4\n"
    "   25: OR         r17, r15, r16\n"
    "   26: SET_ALIAS  a2, r17\n"
    "   27: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,11] --> 2\n"
    "Block 1: <none> --> [12,13] --> 2\n"
    "Block 2: 1,0 --> [14,27] --> <none>\n"
    ;


/* Tweaked version of tests/rtl-disasm-test.i to trigger OOM. */

#include "src/guest-ppc.h"
#include "tests/common.h"
#include "tests/log-capture.h"
#include "tests/mem-wrappers.h"

int main(void)
{
    void *aligned_input;
    ASSERT(aligned_input = malloc(sizeof(input)));
    memcpy(aligned_input, input, sizeof(input));

    binrec_setup_t final_setup = setup;
    final_setup.guest_memory_base = aligned_input;
    final_setup.malloc = mem_wrap_malloc;
    final_setup.realloc = mem_wrap_realloc;
    final_setup.free = mem_wrap_free;
    final_setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&final_setup));

    binrec_set_optimization_flags(handle, 0, guest_opt, 0);
    binrec_set_code_range(handle, 0, sizeof(input) - 1);
    #ifdef BRANCH_CALLBACK
        binrec_enable_branch_exit_test(handle, true);
    #endif
    #ifdef PRE_INSN_CALLBACK
        binrec_set_pre_insn_callback(handle, PRE_INSN_CALLBACK);
    #endif
    #ifdef POST_INSN_CALLBACK
        binrec_set_post_insn_callback(handle, POST_INSN_CALLBACK);
    #endif

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    mem_wrap_fail_after(1);
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
    mem_wrap_cancel_fail();

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
