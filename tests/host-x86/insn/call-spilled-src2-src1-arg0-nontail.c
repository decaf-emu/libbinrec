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
    int dummy_regs[14];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2, reg3;
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[4], 0, 0));  // Free DI.
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    rtl_make_unfoldable(unit, reg1);
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[13], 0, 0));  // R14
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    rtl_make_unfoldable(unit, reg2);
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    rtl_make_unfoldable(unit, reg3);
    for (int i = 0; i < 13; i++) {
        if (i != 4) {
            EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
        }
    }
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, 0, reg1, reg2, reg3));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x20,                // sub $32,%rsp
    0xBF,0x01,0x00,0x00,0x00,           // mov $1,%edi
    0x41,0xBE,0x02,0x00,0x00,0x00,      // mov $2,%r14d
    0x44,0x89,0x34,0x24,                // mov %r14d,(%rsp)
    0x41,0xBE,0x03,0x00,0x00,0x00,      // mov $3,%r14d
    0x48,0x89,0x7C,0x24,0x08,           // mov %rdi,8(%rsp)
    0x0F,0xAE,0x5C,0x24,0x10,           // stmxcsr 16(%rsp)
    0x41,0x8B,0xF6,                     // mov %r14d,%esi
    0x48,0x8B,0xC7,                     // mov %rdi,%rax
    0x8B,0x3C,0x24,                     // mov (%rsp),%edi
    0xFF,0xD0,                          // call *%rax
    0x0F,0xAE,0x54,0x24,0x10,           // ldmxcsr 16(%rsp)
    0x48,0x8B,0x7C,0x24,0x08,           // mov 8(%rsp),%rdi
    0x48,0x83,0xC4,0x20,                // add $32,%rsp
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
