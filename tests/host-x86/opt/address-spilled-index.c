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
static const unsigned int host_opt = BINREC_OPT_H_X86_ADDRESS_OPERANDS
                                   | BINREC_OPT_H_X86_FIXED_REGS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 1));

    int dummy_reg[13];
    for (int i = 0; i < lenof(dummy_reg); i++) {
        EXPECT(dummy_reg[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_reg[i], 0, 0, 0));
    }
    for (int i = 0; i < lenof(dummy_reg); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_reg[i], 0, 0));
    }

    int reg3, reg4, reg5;
    /* Make a copy of reg1 so reg2 has the later death and thus gets
     * spilled. */
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg3, reg1, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg4, reg3, reg2, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg5, reg4, 0, 0));

    /* In order for reg5 not to reuse reg4, we have to use fixed-regs
     * optimization to force it elsewhere.  We use it as a shift count
     * to force it into ECX.*/
    int reg6, reg7;
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg6, reg5, 0, 0));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SLL, reg7, reg6, reg5, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0x48,0x89,0x34,0x24,                // mov %rsi,(%rsp)
    0x48,0x8B,0x04,0x24,                // mov (%rsp),%rax
    0x8B,0x0C,0x07,                     // mov (%rdi,%rax),%ecx
    0x8B,0xC1,                          // mov %ecx,%eax
    0xD3,0xE0,                          // shl %cl,%eax
    0x48,0x83,0xC4,0x10,                // add $16,%rsp
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 29\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] r16 no longer used, setting death = birth\n"
        "[info] Extending r16 live range to 30\n"
        "[info] Extending r2 live range to 30\n"
    #endif
    "";

#include "tests/rtl-translate-test.i"
