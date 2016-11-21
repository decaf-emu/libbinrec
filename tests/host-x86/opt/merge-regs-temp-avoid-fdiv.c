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
static const unsigned int host_opt = BINREC_OPT_H_X86_MERGE_REGS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, alias;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_V2_FLOAT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40000000));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0x40400000));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VBROADCAST, reg4, reg3, 0, 0));  // Gets XMM1.
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg4, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));

    int reg5, reg6, reg7, reg8, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 0x40A00000));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VBROADCAST, reg6, reg5, 0, 0));  // Gets XMM0.
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    /* The temporary for the conversion should not clobber XMM1. */
    EXPECT(rtl_add_insn(unit, RTLOP_FDIV, reg7, reg6, reg6, 0));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg8, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg8, 0, alias));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xEB,0x1A,                          // jmp 0x20
    0x00,0x00,0x00,0x00,0x00,0x00,0x00, // (padding)
    0x00,0x00,0x00,                     // (padding)

    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // (data)
    0x00,0x00,0x80,0x3F,0x00,0x00,0x80,0x3F, // (data)

    0xB8,0x00,0x00,0x00,0x40,           // mov $0x40000000,%eax
    0x66,0x0F,0x6E,0xC0,                // movd %eax,%xmm0
    0xB8,0x00,0x00,0x40,0x40,           // mov $0x40400000,%eax
    0x66,0x0F,0x6E,0xC8,                // movd %eax,%xmm1
    0x0F,0x14,0xC9,                     // unpcklps %xmm1,%xmm1
    0xF3,0x0F,0x7E,0xC9,                // movq %xmm1,%xmm1
    0xB8,0x00,0x00,0xA0,0x40,           // mov $0x40A00000,%eax
    0x66,0x0F,0x6E,0xC0,                // movd %eax,%xmm0
    0x0F,0x14,0xC0,                     // unpcklps %xmm0,%xmm0
    0xF3,0x0F,0x7E,0xC0,                // movq %xmm0,%xmm0
    0x0F,0x28,0xD0,                     // movaps %xmm0,%xmm2
    0x0F,0x56,0x15,0xBD,0xFF,0xFF,0xFF, // orps -67(%rip),%xmm2
    0x0F,0x5E,0xC2,                     // divps %xmm2,%xmm0
    0xF2,0x0F,0x11,0x8F,0x34,0x12,0x00, // movsd %xmm1,0x1234(%rdi)
      0x00,
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
