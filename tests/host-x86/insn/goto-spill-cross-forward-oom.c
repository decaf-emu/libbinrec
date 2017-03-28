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
    /* Allocate enough dummy registers to use 128 bytes of stack space
     * and force subsequent spills to use 32-bit displacements. */
    int regs1[30];
    for (int i = 0; i < lenof(regs1); i++) {
        EXPECT(regs1[i] = rtl_alloc_register(unit, RTLTYPE_INT64));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, regs1[i], 0, 0, 0));
    }

    int label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));

    /* Generate enough jumped spills to trigger OOM, but leave at least
     * one unspilled register so we test both code paths. */
    int regs2[13];
    for (int i = 0; i < lenof(regs2); i++) {
        EXPECT(regs2[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, regs2[i], 0, 0, 0));
    }
    for (int i = 0; i < lenof(regs2); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, regs2[i], 0, 0));
    }

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));

    for (int i = 0; i < lenof(regs1); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, regs1[i], 0, 0));
    }

    return EXIT_SUCCESS;
}

#define EXPECT_TRANSLATE_FAILURE

static const char expected_log[] =
    "[error] No memory for register reload for block 0->2\n"
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

    handle->code_buffer_size = 192;
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
