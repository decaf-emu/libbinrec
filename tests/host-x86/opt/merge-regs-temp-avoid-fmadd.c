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
static const unsigned int host_opt = BINREC_OPT_H_X86_MERGE_REGS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, alias;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_FLOAT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40000000));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0x40400000));  // Gets XMM1.
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));

    int reg4, reg5, reg6, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 0x40800000));  // Spilled.
    int spillers[14];
    for (int i = 0; i < lenof(spillers); i++) {
        spillers[i] = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_NOP, spillers[i], 0, 0, 0);
    }
    for (int i = 0; i < lenof(spillers); i++) {
        rtl_add_insn(unit, RTLOP_NOP, 0, spillers[i], 0, 0);
    }
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    /* The temporary for reloading reg4 should not clobber XMM1. */
    EXPECT(rtl_add_insn(unit, RTLOP_FMADD, reg5, reg4, reg4, reg4));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg6, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg6, 0, alias));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x00,0x00,0x00,0x40,           // mov $0x40000000,%eax
    0x66,0x0F,0x6E,0xC0,                // movd %eax,%xmm0
    0xB8,0x00,0x00,0x40,0x40,           // mov $0x40400000,%eax
    0x66,0x0F,0x6E,0xC8,                // movd %eax,%xmm1
    0xB8,0x00,0x00,0x80,0x40,           // mov $0x40800000,%eax
    0x66,0x0F,0x6E,0xC0,                // movd %eax,%xmm0
    0xF3,0x0F,0x11,0x04,0x24,           // movss %xmm0,(%rsp)
    0xF3,0x0F,0x10,0x04,0x24,           // movss (%rsp),%xmm0
    0xF3,0x0F,0x10,0x14,0x24,           // movss (%rsp),%xmm2
    0xC4,0xE2,0x69,0xA9,0x04,0x24,      // vfmadd213ss (%rsp),%xmm2,%xmm0
    0xF3,0x0F,0x11,0x8F,0x34,0x12,0x00, // movss %xmm1,0x1234(%rdi)
      0x00,
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
