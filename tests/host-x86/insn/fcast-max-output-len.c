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
    int dummy_alias[8];
    for (int i = 0; i < lenof(dummy_alias); i++) {
        EXPECT(dummy_alias[i] = rtl_alloc_alias_register(unit,
                                                         RTLTYPE_V2_FLOAT64));
    }

    alloc_dummy_registers(unit, 5, RTLTYPE_INT32);
    int dummy_regs[14];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_FCAST, reg2, reg1, 0, 0));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x81,0xEC,0x88,0x00,0x00,0x00,      // sub $136,%rsp
    0x41,0xB8,0x00,0x00,0x80,0x3F,           // mov $0x3F800000,%r8d
    0x66,0x45,0x0F,0x6E,0xF0,                // movd %r8d,%xmm14
    0xF3,0x44,0x0F,0x11,0xB4,0x24,0x84,0x00, // movss %xmm14,132(%rsp)
      0x00,0x00,
    0x0F,0xAE,0x9C,0x24,0x80,0x00,0x00,0x00, // stmxcsr 128(%rsp)
    0x44,0x8B,0x84,0x24,0x80,0x00,0x00,0x00, // mov 128(%rsp),%r8d
    0x83,0xA4,0x24,0x80,0x00,0x00,0x00,0xFE, // andl $-2,128(%rsp)
    0x0F,0xAE,0x94,0x24,0x80,0x00,0x00,0x00, // ldmxcsr 128(%rsp)
    0xF3,0x44,0x0F,0x5A,0xB4,0x24,0x84,0x00, // cvtss2sd 132(%rsp),%xmm14
      0x00,0x00,
    0x0F,0xAE,0x9C,0x24,0x80,0x00,0x00,0x00, // stmxcsr 128(%rsp)
    0xF6,0x84,0x24,0x80,0x00,0x00,0x00,0x01, // testb $1,128(%rsp)
    0x44,0x89,0x84,0x24,0x80,0x00,0x00,0x00, // mov %r8d,128(%rsp)
    0x0F,0xAE,0x94,0x24,0x80,0x00,0x00,0x00, // ldmxcsr 128(%rsp)
    0x74,0x13,                               // jz L0
    0x49,0xB8,0xFF,0xFF,0xFF,0xFF,0xFF,      // mov $0xFFF7FFFFFFFFFFFF,%r8
      0xFF,0xF7,0xFF,
    0x66,0x4D,0x0F,0x6E,0xF8,                // movq %r8,%xmm15
    0x45,0x0F,0x54,0xF7,                     // andps %xmm15,%xmm14
    0x48,0x81,0xC4,0x88,0x00,0x00,0x00,      // add $136,%rsp
    0xC3,                                    // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
