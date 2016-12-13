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
    alloc_dummy_registers(unit, 5, RTLTYPE_INT32);
    alloc_dummy_registers(unit, 9, RTLTYPE_FLOAT32);

    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg1, 0, 0, UINT64_C(0x3FF0000000000000)));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg2, 0, 0, UINT64_C(0x4000000000000000)));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_VBROADCAST, reg3, reg1, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_VBROADCAST, reg4, reg2, 0, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_VFCMP, reg5, reg3, reg4, RTLFCMP_UN));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, reg4, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x49,0xB8,0x00,0x00,0x00,0x00,0x00, // mov $0x3FF0000000000000,%r8
      0x00,0xF0,0x3F,
    0x66,0x4D,0x0F,0x6E,0xC8,           // movq %r8,%xmm9
    0x49,0xB8,0x00,0x00,0x00,0x00,0x00, // mov $0x4000000000000000,%r8
      0x00,0x00,0x40,
    0x66,0x4D,0x0F,0x6E,0xD0,           // movq %r8,%xmm10
    0xF2,0x45,0x0F,0x12,0xC9,           // movddup %xmm9,%xmm9
    0xF2,0x45,0x0F,0x12,0xD2,           // movddup %xmm10,%xmm10
    0x45,0x0F,0x28,0xD9,                // movaps %xmm9,%xmm11
    0x66,0x45,0x0F,0xC2,0xDA,0x03,      // cmpunordpd %xmm10,%xmm11
    0x66,0x41,0x0F,0x73,0xDB,0x04,      // psrldq $4,%xmm11
    0x66,0x4D,0x0F,0x7E,0xD8,           // movq %xmm11,%r8
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
