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
    .host_features = BINREC_FEATURE_X86_FMA,
};
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    int dummy_regs[12];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2, reg3, spiller, spiller2, spiller3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40000000));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0x40400000));
    EXPECT(spiller = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, spiller, 0, 0, 0x40800000));
    EXPECT(spiller2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, spiller2, 0, 0, 0x40A00000));
    EXPECT(spiller3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, spiller3, 0, 0, 0x40C00000));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FMADD, reg4, reg1, reg2, reg3));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, spiller, spiller2, 0));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x18,                // sub $24,%rsp
    0xB8,0x00,0x00,0x80,0x3F,           // mov $0x3F800000,%eax
    0x66,0x44,0x0F,0x6E,0xE0,           // movd %eax,%xmm12
    0xB8,0x00,0x00,0x00,0x40,           // mov $0x40000000,%eax
    0x66,0x44,0x0F,0x6E,0xE8,           // movd %eax,%xmm13
    0xB8,0x00,0x00,0x40,0x40,           // mov $0x40400000,%eax
    0x66,0x44,0x0F,0x6E,0xF0,           // movd %eax,%xmm14
    0xF3,0x44,0x0F,0x11,0x24,0x24,      // movss %xmm12,(%rsp)
    0xB8,0x00,0x00,0x80,0x40,           // mov $0x40800000,%eax
    0x66,0x44,0x0F,0x6E,0xE0,           // movd %eax,%xmm12
    0xF3,0x44,0x0F,0x11,0x6C,0x24,0x04, // movss %xmm13,4(%rsp)
    0xB8,0x00,0x00,0xA0,0x40,           // mov $0x40A00000,%eax
    0x66,0x44,0x0F,0x6E,0xE8,           // movd %eax,%xmm13
    0xF3,0x44,0x0F,0x11,0x74,0x24,0x08, // movss %xmm14,8(%rsp)
    0xB8,0x00,0x00,0xC0,0x40,           // mov $0x40C00000,%eax
    0x66,0x44,0x0F,0x6E,0xF0,           // movd %eax,%xmm14
    0xF3,0x44,0x0F,0x10,0x74,0x24,0x04, // movss 4(%rsp),%xmm14
    0xF3,0x44,0x0F,0x10,0x3C,0x24,      // movss (%rsp),%xmm15
    0xC4,0x62,0x01,0xA9,0x74,0x24,0x08, // vfmadd213ss 8(%rsp),%xmm15,%xmm14
    0x48,0x83,0xC4,0x18,                // add $24,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
