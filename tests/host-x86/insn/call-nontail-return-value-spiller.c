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
    int dummy_regs[13];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    rtl_make_unfoldable(unit, reg1);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* reg1 will still be live when this CALL is processed; that shouldn't
     * cause the translator to try and save/restore it (which would
     * overwrite the return value). */
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, reg2, reg1, 0, 0));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x60,                // sub $96,%rsp
    0x41,0xBB,0x01,0x00,0x00,0x00,      // mov $1,%r11d
    0x4C,0x89,0x5C,0x24,0x50,           // mov %r11,80(%rsp)
    0x48,0x89,0x04,0x24,                // mov %rax,(%rsp)
    0x48,0x89,0x4C,0x24,0x08,           // mov %rcx,8(%rsp)
    0x48,0x89,0x54,0x24,0x10,           // mov %rdx,16(%rsp)
    0x48,0x89,0x74,0x24,0x18,           // mov %rsi,24(%rsp)
    0x48,0x89,0x7C,0x24,0x20,           // mov %rdi,32(%rsp)
    0x4C,0x89,0x44,0x24,0x28,           // mov %r8,40(%rsp)
    0x4C,0x89,0x4C,0x24,0x30,           // mov %r9,48(%rsp)
    0x4C,0x89,0x54,0x24,0x38,           // mov %r10,56(%rsp)
    0x0F,0xAE,0x5C,0x24,0x48,           // stmxcsr 72(%rsp)
    0x48,0x8B,0x44,0x24,0x50,           // mov 80(%rsp),%rax
    0xFF,0xD0,                          // call *%rax
    0x44,0x8B,0xD8,                     // mov %eax,%r11d
    0x0F,0xAE,0x54,0x24,0x48,           // ldmxcsr 72(%rsp)
    0x48,0x8B,0x04,0x24,                // mov (%rsp),%rax
    0x48,0x8B,0x4C,0x24,0x08,           // mov 8(%rsp),%rcx
    0x48,0x8B,0x54,0x24,0x10,           // mov 16(%rsp),%rdx
    0x48,0x8B,0x74,0x24,0x18,           // mov 24(%rsp),%rsi
    0x48,0x8B,0x7C,0x24,0x20,           // mov 32(%rsp),%rdi
    0x4C,0x8B,0x44,0x24,0x28,           // mov 40(%rsp),%r8
    0x4C,0x8B,0x4C,0x24,0x30,           // mov 48(%rsp),%r9
    0x4C,0x8B,0x54,0x24,0x38,           // mov 56(%rsp),%r10
    0x48,0x83,0xC4,0x60,                // add $96,%rsp
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
