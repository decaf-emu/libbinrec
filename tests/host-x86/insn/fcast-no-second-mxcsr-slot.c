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
    alloc_dummy_registers(unit, 1, RTLTYPE_FLOAT32);

    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg1, 0, 0, UINT64_C(0x3FF0000000000000)));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCAST, reg2, reg1, 0, 0));
    /* This should reuse the existing frame slot for stmxcsr/ldmxcsr. */
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_FCAST, reg3, reg2, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x48,0xB8,0x00,0x00,0x00,0x00,0x00, // mov $0x3FF0000000000000,%rax
      0x00,0xF0,0x3F,
    0x66,0x48,0x0F,0x6E,0xC8,           // movq %rax,%xmm1
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x8B,0x04,0x24,                     // mov (%rsp),%eax
    0x83,0x24,0x24,0xFE,                // andl $-2,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0xF2,0x0F,0x5A,0xC9,                // cvtsd2ss %xmm1,%xmm1
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0xF6,0x04,0x24,0x01,                // testb $1,(%rsp)
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x74,0x0C,                          // jz L0
    0xB8,0xFF,0xFF,0xBF,0xFF,           // mov $0xFFBFFFFF,%eax
    0x66,0x0F,0x6E,0xD0,                // movd %eax,%xmm2
    0x0F,0x54,0xCA,                     // andps %xmm2,%xmm1
    0x0F,0xAE,0x1C,0x24,                // L0: stmxcsr (%rsp)
    0x8B,0x04,0x24,                     // mov (%rsp),%eax
    0x83,0x24,0x24,0xFE,                // andl $-2,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0xF3,0x0F,0x5A,0xC9,                // cvtss2sd %xmm1,%xmm1
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0xF6,0x04,0x24,0x01,                // testb $1,(%rsp)
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x74,0x12,                          // jz L0
    0x48,0xB8,0xFF,0xFF,0xFF,0xFF,0xFF, // mov $0xFFF7FFFFFFFFFFFF,%rax
      0xFF,0xF7,0xFF,
    0x66,0x48,0x0F,0x6E,0xD0,           // movq %rax,%xmm2
    0x0F,0x54,0xCA,                     // andps %xmm2,%xmm1
    0x48,0x83,0xC4,0x08,                // L0: add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
