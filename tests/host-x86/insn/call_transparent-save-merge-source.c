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
    int reg1, alias;
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg1, 0, alias));

    int reg2, reg3, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    /* This call should preserve reg1 (which has been moved to ECX). */
    EXPECT(rtl_add_insn(unit, RTLOP_CALL_TRANSPARENT, 0, reg2, 0, 0));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg3, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x18,                // sub $24,%rsp
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0x8B,0xC8,                          // mov %eax,%ecx
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0x48,0x89,0x4C,0x24,0x08,           // mov %rcx,8(%rsp)
    0xFF,0xD0,                          // call *%rax
    0x48,0x8B,0x4C,0x24,0x08,           // mov 8(%rsp),%rcx
    0x89,0x0C,0x24,                     // mov %ecx,(%rsp)
    0x48,0x83,0xC4,0x18,                // add $24,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
