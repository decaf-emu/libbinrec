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
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg2, reg1, 0, 0));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0x40400000));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VINSERT, reg4, reg2, reg3, 0));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0xF2,0x44,0x0F,0x10,0x28,           // movsd (%rax),%xmm13
    0xB8,0x00,0x00,0x40,0x40,           // mov $0x40400000,%eax
    0x66,0x44,0x0F,0x6E,0xF0,           // movd %eax,%xmm14
    0xF3,0x44,0x0F,0x11,0x34,0x24,      // movss %xmm14,(%rsp)
    0x45,0x0F,0x28,0xF5,                // movaps %xmm13,%xmm14
    0xF3,0x44,0x0F,0x10,0x3C,0x24,      // movss (%rsp),%xmm15
    0x66,0x41,0x0F,0x73,0xF6,0x20,      // psllq $32,%xmm14
    0xF3,0x45,0x0F,0x10,0xF7,           // movss %xmm15,%xmm14
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
