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
    .host = BINREC_ARCH_X86_64_WINDOWS,
};
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    int dummy_regs[1];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    /* Make the register explicitly unfoldable to block the optimization
     * of loading the register at the call site. */
    rtl_make_unfoldable(unit, reg1);
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, 0, reg1, 0, 0));

    /* A second call should not allocate additional shadow space beyond
     * that reserved by the first one. */
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    rtl_make_unfoldable(unit, reg2);
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, 0, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    /* This should also reserve shadow space for the callee (even though
     * we don't pass any arguments). */
    0x48,0x83,0xEC,0x28,                // sub $40,%rsp
    0xB9,0x01,0x00,0x00,0x00,           // mov $1,%ecx
    0x0F,0xAE,0x5C,0x24,0x20,           // stmxcsr 32(%rsp)
    0xFF,0xD1,                          // call *%rcx
    0x0F,0xAE,0x54,0x24,0x20,           // ldmxcsr 32(%rsp)
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0x0F,0xAE,0x5C,0x24,0x20,           // stmxcsr 32(%rsp)
    0xFF,0xD0,                          // call *%rax
    0x0F,0xAE,0x54,0x24,0x20,           // ldmxcsr 32(%rsp)
    0x48,0x83,0xC4,0x28,                // add $40,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
