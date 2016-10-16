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
    int reg1;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));

    int label, regs[13], reg2;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, regs[i], 0, 0, 0));
    }
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));  // Spills reg1.
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, regs[i], 0, 0));
    }
    /* reg2 is live past this branch, causing a reload conflict with reg1. */
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg1, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 1));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0x89,0x04,0x24,                     // L1: mov %eax,(%rsp)
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0x83,0x3C,0x24,0x00,                // cmpl $0,(%rsp)
    0x75,0x05,                          // jnz L0
    0x8B,0x04,0x24,                     // mov (%rsp),%eax
    0xEB,0xED,                          // jmp L1
    0x0F,0x1F,0x05,0x01,0x00,0x00,0x00, // L0: nop 1(%rip)
    0x48,0x83,0xC4,0x10,                // add $16,%rsp
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
