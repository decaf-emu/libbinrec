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
    alloc_dummy_registers(unit, 8, RTLTYPE_INT32);
    alloc_dummy_registers(unit, 8, RTLTYPE_FLOAT32);

    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg1, 0, 0, UINT64_C(0x3FF0000000000000)));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCAST, reg2, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x49,0xBB,0x00,0x00,0x00,0x00,0x00, // mov $0x3FF0000000000000,%r11
      0x00,0xF0,0x3F,
    0x66,0x4D,0x0F,0x6E,0xC3,           // movq %r11,%xmm8
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x44,0x8B,0x1C,0x24,                // mov (%rsp),%r11d
    0x83,0x24,0x24,0xFE,                // andl $-2,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0xF2,0x45,0x0F,0x5A,0xC8,           // cvtsd2ss %xmm8,%xmm9
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0xF6,0x04,0x24,0x01,                // testb $1,(%rsp)
    0x44,0x89,0x1C,0x24,                // mov %r11d,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x74,0x0F,                          // jz L0
    0x41,0xBB,0xFF,0xFF,0xBF,0xFF,      // mov $0xFFBFFFFF,%r11d
    0x66,0x45,0x0F,0x6E,0xD3,           // movd %r11d,%xmm10
    0x45,0x0F,0x54,0xCA,                // andps %xmm10,%xmm9
    0x48,0x83,0xC4,0x08,                // L0: add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
