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
    alloc_dummy_registers(unit, 1, RTLTYPE_INT32);

    int reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8, reg9;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg3, reg1, reg2, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg4, reg3, 0, 0));
    /* None of these immediates should clobber RSI/RDI. */
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 5));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg6, 0, 0, 6));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg7, 0, 0, 7));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg8, 0, 0, 8));
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg9, reg1, reg2, reg4));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg5, reg6, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg7, reg8, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg9, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x8B,0x0C,0x37,                     // mov (%rdi,%rsi),%ecx
    0xBA,0x05,0x00,0x00,0x00,           // mov $5,%edx
    0x41,0xB8,0x06,0x00,0x00,0x00,      // mov $6,%r8d
    0x41,0xB9,0x07,0x00,0x00,0x00,      // mov $7,%r9d
    0x41,0xBA,0x08,0x00,0x00,0x00,      // mov $8,%r10d
    0x85,0xC9,                          // test %ecx,%ecx
    0x48,0x0F,0x44,0xFE,                // cmovz %rsi,%rdi
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
