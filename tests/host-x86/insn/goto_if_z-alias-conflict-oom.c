/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/common.h"
#include "tests/host-x86/common.h"
#include "tests/mem-wrappers.h"


static const binrec_setup_t setup = {
    .host = BINREC_ARCH_X86_64_SYSV,
    .code_malloc = mem_wrap_code_malloc,
    .code_realloc = mem_wrap_code_realloc,
    .code_free = mem_wrap_code_free,
};
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, alias, label1;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(label1 = rtl_alloc_label(unit));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));  // Gets EAX.
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));  // Gets ECX.
    /* Add a bunch of NOPs to pad the code past the length reserved for
     * the prologue. */
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 2));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 3));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 4));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 5));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 6));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 7));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 8));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 9));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 11));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 12));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg3, 0, label1));

    int reg4, label2;
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* Allocate ECX (live through the end of the unit) to prevent merging
     * to the same register. */
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 4));
    /* Force EAX to be live past the conditional branch. */
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 1));
    /* Don't fall through so that the GET_ALIAS below can be merged without
     * loading. */
    EXPECT(label2 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label2));

    /* This block is unreachable, but we set up a bunch of aliases here
     * which are merged into the next block in order to pad the alias setup
     * code to the point where it will exceed the available space in the
     * output buffer.  (We need to pad the setup code because the
     * translator always reserves a fixed size, currently 45 bytes (see
     * MAX_INSN_LEN), for all instructions.) */
    int pad_alias[6], pad_reg[6];
    STATIC_ASSERT(lenof(pad_alias) == lenof(pad_reg), "Array length mismatch");
    for (int i = 0; i < lenof(pad_alias); i++) {
        EXPECT(pad_alias[i] = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
        rtl_set_alias_storage(unit, pad_alias[i], reg1, 0x100+i*4);
        EXPECT(pad_reg[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, pad_reg[i], 0, 0, 0));
        EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS,
                            0, pad_reg[i], 0, pad_alias[i]));
    }

    int reg5;
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label1));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* reg5 should be merged with reg3 via EAX.  Since EAX is live past the
     * conditional branch above, the merge should trigger an alias conflict
     * which will flip the sense of the conditional branch so the merge can
     * be performed conditionally. */
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg5, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg5, 0, alias));
    /* Also reload the padding aliases to trigger merging. */
    for (int i = 0; i < lenof(pad_alias); i++) {
        int reg;
        EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, pad_alias[i]));
    }

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg4, 0));
    return EXIT_SUCCESS;
}

#define EXPECT_TRANSLATE_FAILURE

static const char expected_log[] =
    "[error] No memory for alias conflict resolution code\n"
    "[error] Out of memory while generating code\n";


/* Tweaked version of tests/rtl-translate-test.i to trigger OOM. */

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
        printf("%s:%d: add_rtl(unit) failed (%s)\n%s", __FILE__, line,
               log_messages ? "log follows" : "no errors logged",
               log_messages ? log_messages : "");
        return EXIT_FAILURE;
    }

    EXPECT(rtl_finalize_unit(unit));

    /* We have to set the buffer to a precise size to trigger the
     * buffer-full logic in alias conflict resolution. */
    handle->code_buffer_size = 154;
    handle->code_alignment = 16;
    EXPECT(handle->code_buffer = binrec_code_malloc(
               handle, handle->code_buffer_size, handle->code_alignment));

    mem_wrap_code_fail_after(0);

    #ifndef EXPECT_TRANSLATE_FAILURE
        EXPECT(host_x86_translate(handle, unit));
        if (!(handle->code_len == sizeof(expected_code)
              && memcmp(handle->code_buffer, expected_code,
                        sizeof(expected_code)) == 0)) {
            const char *log_messages = get_log_messages();
            if (log_messages) {
                fputs(log_messages, stdout);
            }
            EXPECT_MEMEQ(handle->code_buffer, expected_code,
                         sizeof(expected_code));
            EXPECT_EQ(handle->code_len, sizeof(expected_code));
        }
    #else
        EXPECT_FALSE(host_x86_translate(handle, unit));
    #endif

    const char *log_messages = get_log_messages();
    EXPECT_STREQ(log_messages, *expected_log ? expected_log : NULL);

    binrec_code_free(handle, handle->code_buffer);
    handle->code_buffer = NULL;
    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
