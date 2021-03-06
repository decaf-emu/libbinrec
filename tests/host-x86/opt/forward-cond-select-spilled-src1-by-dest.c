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
static const unsigned int host_opt = BINREC_OPT_H_X86_FORWARD_CONDITIONS;

static int add_rtl(RTLUnit *unit)
{
    int dummy_regs[12];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SGTU, reg3, reg1, reg2, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* A temporary should be allocated even though reg1 won't be spilled
     * until after dest gets its host register (which spills reg1). */
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg4, reg1, reg2, reg3));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));  // Force reg1 spill.

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0x41,0xBD,0x01,0x00,0x00,0x00,      // L1: mov $1,%r13d
    0x41,0xBE,0x02,0x00,0x00,0x00,      // mov $2,%r14d
    0x44,0x89,0x2C,0x24,                // mov %r13d,(%rsp)
    0x44,0x8B,0x2C,0x24,                // mov (%rsp),%r13d
    0x45,0x3B,0xEE,                     // cmp %r14d,%r13d
    0x44,0x8B,0x2C,0x24,                // mov (%rsp),%r13d
    0x45,0x0F,0x46,0xEE,                // cmovbe %r14d,%r13d
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
        "[info] Killing instruction 14\n"
    #endif
    "";

#include "tests/rtl-translate-test.i"
