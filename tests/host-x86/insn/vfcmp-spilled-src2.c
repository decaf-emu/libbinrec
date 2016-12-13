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

    int reg1, reg2, reg3, reg4, reg5, spiller;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40000000));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VBROADCAST, reg3, reg1, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VBROADCAST, reg4, reg2, 0, 0));
    EXPECT(spiller = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, spiller, 0, 0, 0x7F800000));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_VFCMP, reg5, reg3, reg4, RTLFCMP_UN));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg4, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x18,                // sub $24,%rsp
    0xB8,0x00,0x00,0x80,0x3F,           // mov $0x3F800000,%eax
    0x66,0x44,0x0F,0x6E,0xE8,           // movd %eax,%xmm13
    0xB8,0x00,0x00,0x00,0x40,           // mov $0x40000000,%eax
    0x66,0x44,0x0F,0x6E,0xF0,           // movd %eax,%xmm14
    0x45,0x0F,0x14,0xED,                // unpcklps %xmm13,%xmm13
    0xF3,0x45,0x0F,0x7E,0xED,           // movq %xmm13,%xmm13
    0x45,0x0F,0x14,0xF6,                // unpcklps %xmm14,%xmm14
    0xF3,0x45,0x0F,0x7E,0xF6,           // movq %xmm14,%xmm14
    0x44,0x0F,0x29,0x34,0x24,           // movaps %xmm14,(%rsp)
    0xB8,0x00,0x00,0x80,0x7F,           // mov $0x7F800000,%eax
    0x66,0x44,0x0F,0x6E,0xF0,           // movd %eax,%xmm14
    0x45,0x0F,0x28,0xF5,                // movaps %xmm13,%xmm14
    0x44,0x0F,0xC2,0x34,0x24,0x03,      // cmpunordps (%rsp),%xmm14
    0x66,0x4C,0x0F,0x7E,0xF0,           // movq %xmm14,%rax
    0x48,0x83,0xC4,0x18,                // add $24,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
