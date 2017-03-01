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
    alloc_dummy_registers(unit, 1, RTLTYPE_INT32);
    alloc_dummy_registers(unit, 2, RTLTYPE_FLOAT32);

    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40000000));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VBROADCAST, reg3, reg1, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VBROADCAST, reg4, reg2, 0, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_VFCMP, reg5, reg3, reg4, RTLFCMP_UN));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, reg4, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB9,0x00,0x00,0x80,0x3F,           // mov $0x3F800000,%ecx
    0x66,0x0F,0x6E,0xD1,                // movd %ecx,%xmm2
    0xB9,0x00,0x00,0x00,0x40,           // mov $0x40000000,%ecx
    0x66,0x0F,0x6E,0xD9,                // movd %ecx,%xmm3
    0x0F,0x14,0xD2,                     // unpcklps %xmm2,%xmm2
    0xF3,0x0F,0x7E,0xD2,                // movq %xmm2,%xmm2
    0x0F,0x14,0xDB,                     // unpcklps %xmm3,%xmm3
    0xF3,0x0F,0x7E,0xDB,                // movq %xmm3,%xmm3
    0x0F,0x28,0xE2,                     // movaps %xmm2,%xmm4
    0x0F,0xC2,0xE3,0x03,                // cmpunordps %xmm3,%xmm4
    0x66,0x48,0x0F,0x7E,0xE1,           // movq %xmm4,%rcx
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"