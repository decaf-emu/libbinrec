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
    int reg1, reg2, reg3, reg4, alias1, alias2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias1 = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias1, reg1, 0x1234);
    EXPECT(alias2 = rtl_alloc_alias_register(unit, RTLTYPE_FLOAT32));
    rtl_set_alias_storage(unit, alias2, reg1, 0x5678);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));  // Gets EAX.
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0x40400000));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 0x40800000));  // Gets XMM1.
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg4, 0, alias2));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));

    int reg5, reg6, reg7, reg8, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 0x40A00000));  // Gets XMM0.
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    /* The temporary for the conversion should not clobber EAX or XMM1. */
    EXPECT(rtl_add_insn(unit, RTLOP_FCAST, reg6, reg5, 0, 0));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg7, 0, 0, alias1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg7, 0, alias1));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg8, 0, 0, alias2));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg8, 0, alias2));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0xB9,0x00,0x00,0x40,0x40,           // mov $0x40400000,%ecx
    0x66,0x0F,0x6E,0xC1,                // movd %ecx,%xmm0
    0xB9,0x00,0x00,0x80,0x40,           // mov $0x40800000,%ecx
    0x66,0x0F,0x6E,0xC9,                // movd %ecx,%xmm1
    0xB9,0x00,0x00,0xA0,0x40,           // mov $0x40A00000,%ecx
    0x66,0x0F,0x6E,0xC1,                // movd %ecx,%xmm0
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x8B,0x0C,0x24,                     // mov (%rsp),%ecx
    0x83,0x24,0x24,0xFE,                // andl $-2,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0xF3,0x0F,0x5A,0xC0,                // cvtss2sd %xmm0,%xmm0
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0xF6,0x04,0x24,0x01,                // testb $1,(%rsp)
    0x89,0x0C,0x24,                     // mov %ecx,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x74,0x12,                          // jz L0
    0x48,0xB9,0xFF,0xFF,0xFF,0xFF,0xFF, // mov $0xFFF7FFFFFFFFFFFF,%rcx
      0xFF,0xF7,0xFF,
    0x66,0x48,0x0F,0x6E,0xD1,           // movq %rcx,%xmm2
    0x0F,0x54,0xC2,                     // andps %xmm2,%xmm0
    0x89,0x87,0x34,0x12,0x00,0x00,      // L0: mov %eax,0x1234(%rdi)
    0xF3,0x0F,0x11,0x8F,0x78,0x56,0x00, // movss %xmm1,0x5678(%rdi)
      0x00,
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
