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

    int reg1;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));

    /* Top end of range. */
    int reg2, reg3;
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg2, reg1, 0, 0x7FFFFFFE));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg3, reg2, 0, 1));

    /* Bottom end of range. */
    int reg4, reg5;
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg4, reg1, 0, -0x7FFFFFFF));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg5, reg4, 0, -1));

    /* Beyond top end of range. */
    int reg6, reg7;
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg6, reg1, 0, 0x7FFFFFFF));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg7, reg6, 0, 1));

    /* Beyond bottom end of range. */
    int reg8, reg9;
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg8, reg1, 0, -INT64_C(0x80000000)));
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg9, reg8, 0, -1));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x8B,0x8F,0xFF,0xFF,0xFF,0x7F,      // mov 0x7FFFFFFF(%rdi),%ecx
    0x8B,0x8F,0x00,0x00,0x00,0x80,      // mov -0x80000000(%rdi),%ecx
    0x48,0x8B,0xCF,                     // mov %rdi,%rcx
    0x48,0x81,0xC1,0xFF,0xFF,0xFF,0x7F, // add $0x7FFFFFFF,%rcx
    0x8B,0x49,0x01,                     // mov 1(%rcx),%ecx
    0x48,0x81,0xC7,0x00,0x00,0x00,0x80, // add $-0x80000000,%rdi
    0x8B,0x7F,0xFF,                     // mov -1(%rdi),%edi
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 2\n"
        "[info] Killing instruction 4\n"
    #endif
    "";

#include "tests/rtl-translate-test.i"
