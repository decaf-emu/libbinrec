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
    int reg1, reg2, alias;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_FLOAT));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40000000));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias));

    int reg3, label, tempreg[15];
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    for (int i = 0; i < lenof(tempreg); i++) {
        EXPECT(tempreg[i] = rtl_alloc_register(unit, RTLTYPE_FLOAT));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, tempreg[i], 0, 0, 0));
    }
    for (int i = 0; i < lenof(tempreg); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, tempreg[i], 0, 0));
    }
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg3, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x00,0x00,0x00,0x40,           // mov $0x40000000,%eax
    0x66,0x0F,0x6E,0xC0,                // movd %eax,%xmm0
    0xF3,0x0F,0x11,0x87,0x34,0x12,0x00, // movss %xmm0,0x1234(%rdi)
      0x00,
    0x0F,0x57,0xC0,                     // xorps %xmm0,%xmm0
    0x0F,0x57,0xC9,                     // xorps %xmm1,%xmm1
    0x0F,0x57,0xD2,                     // xorps %xmm2,%xmm2
    0x0F,0x57,0xDB,                     // xorps %xmm3,%xmm3
    0x0F,0x57,0xE4,                     // xorps %xmm4,%xmm4
    0x0F,0x57,0xED,                     // xorps %xmm5,%xmm5
    0x0F,0x57,0xF6,                     // xorps %xmm6,%xmm6
    0x0F,0x57,0xFF,                     // xorps %xmm7,%xmm7
    0x45,0x0F,0x57,0xC0,                // xorps %xmm8,%xmm8
    0x45,0x0F,0x57,0xC9,                // xorps %xmm9,%xmm9
    0x45,0x0F,0x57,0xD2,                // xorps %xmm10,%xmm10
    0x45,0x0F,0x57,0xDB,                // xorps %xmm11,%xmm11
    0x45,0x0F,0x57,0xE4,                // xorps %xmm12,%xmm12
    0x45,0x0F,0x57,0xED,                // xorps %xmm13,%xmm13
    0x45,0x0F,0x57,0xF6,                // xorps %xmm14,%xmm14
    0xF3,0x0F,0x10,0x87,0x34,0x12,0x00, // movss 0x1234(%rdi),%xmm0
      0x00,
    0xF3,0x0F,0x11,0x87,0x34,0x12,0x00, // movss %xmm0,0x1234(%rdi)
      0x00,
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
