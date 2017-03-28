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


static const binrec_setup_t setup = {
    .host = BINREC_ARCH_X86_64_SYSV,
};
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    int reg1, alias, label;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(label = rtl_alloc_label(unit));

    for (int i = 1; i <= 7; i++) {
        int regA, regB;
        EXPECT(regA = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, regA, 0, 0, alias));
        EXPECT(regB = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_ADDI, regB, regA, 0, -1));
        EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, regB, 0, alias));
        if (i < 7) {
            EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, regB, 0, label));
        } else {
            EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));
        }
    }

    /* This block will be dropped by BINREC_OPT_BASIC, leaving the entries[]
     * array empty.  That should not trick the code into thinking this is
     * the unit's entry block. */
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));

    int reg2, reg3, reg4;
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg2, 0, 0, alias));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SLLI, reg3, reg2, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));
    /* This should not clobber reg3. */
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 4));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x8B,0x87,0x34,0x12,0x00,0x00,      // mov 0x1234(%rdi),%eax
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x45,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x3A,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x2F,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x24,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x19,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x0E,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)
    0xE9,0x0A,0x00,0x00,0x00,           // jmp L2
    0xC1,0xE0,0x01,                     // L1: shl $1,%eax
    0xB9,0x04,0x00,0x00,0x00,           // mov $4,%ecx
    0xEB,0xF6,                          // jmp L1
    0x48,0x83,0xC4,0x08,                // L2: add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead block 9 (36-36)\n"
        "[info] Killing instruction 36\n"
        "[info] r1 death rolled back to 33\n"
        "[info] Dropping dead block 7 (29-29)\n"
        "[info] Killing instruction 29\n"
    #endif
    "";


/* Tweaked version of tests/rtl-translate-test.i to call rtl_optimize_unit(). */

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

    if (!rtl_finalize_unit(unit)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("rtl_finalize_unit(unit) was not true as expected");
    }

    EXPECT(rtl_optimize_unit(unit, BINREC_OPT_BASIC));

    #ifdef EXPECT_TRANSLATE_FAILURE
        handle->code_buffer_size = 1;
    #else
        handle->code_buffer_size = sizeof(expected_code);
    #endif
    handle->code_alignment = 16;
    EXPECT(handle->code_buffer = binrec_code_malloc(
               handle, handle->code_buffer_size, handle->code_alignment));

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
