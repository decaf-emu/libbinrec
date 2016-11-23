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
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_ADDRESS));
    int dummy_regs[9];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg1, 0, alias));

    int reg2, reg3, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    /* This call should not attempt to preserve reg1, since it's in a
     * callee-saved register. */
    EXPECT(rtl_add_insn(unit, RTLOP_CALL_TRANSPARENT, 0, reg2, 0, 0));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg3, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0xBB,0x01,0x00,0x00,0x00,           // mov $1,%ebx
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0xFF,0xD0,                          // call *%rax
    0x0F,0xAE,0x5C,0x24,0x08,           // stmxcsr 8(%rsp)
    0x83,0x64,0x24,0x08,0xC0,           // and $-64,8(%rsp)
    0x0F,0xAE,0x54,0x24,0x08,           // ldmxcsr 8(%rsp)
    0x48,0x89,0x1C,0x24,                // mov %rbx,(%rsp)
    0x48,0x83,0xC4,0x10,                // add $16,%rsp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
