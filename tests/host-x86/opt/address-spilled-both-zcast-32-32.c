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
static const unsigned int host_opt = BINREC_OPT_H_X86_ADDRESS_OPERANDS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ZCAST, reg3, reg1, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ZCAST, reg4, reg2, 0, 0));

    int dummy_reg[14];
    for (int i = 0; i < lenof(dummy_reg); i++) {
        EXPECT(dummy_reg[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_reg[i], 0, 0, 0));
    }
    for (int i = 0; i < lenof(dummy_reg); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_reg[i], 0, 0));
    }

    int reg5, reg6;
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg5, reg3, reg4, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg6, reg5, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0x89,0x3C,0x24,                     // mov %edi,(%rsp)
    0x89,0x74,0x24,0x04,                // mov %esi,4(%rsp)
    0x8B,0x0C,0x24,                     // mov (%rsp),%ecx
    0x03,0x4C,0x24,0x04,                // add 4(%rsp),%ecx
    0x73,0x0C,                          // jnc L0
    0x48,0xC1,0xC9,0x20,                // ror $32,%rcx
    0x48,0x83,0xC1,0x01,                // add $1,%rcx
    0x48,0xC1,0xC9,0x20,                // ror $32,%rcx
    0x8B,0x01,                          // L0: mov (%rcx),%eax
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
        "[info] Killing instruction 32\n"
        "[info] r4 no longer used, setting death = birth\n"
        "[info] r3 no longer used, setting death = birth\n"
        "[info] Killing instruction 2\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] Killing instruction 3\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] Extending r1 live range to 33\n"
        "[info] Extending r2 live range to 33\n"
    #endif
    "[warning] Slow add of spilled 32-bit base and index at 33\n";

#include "tests/rtl-translate-test.i"
