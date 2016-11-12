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

    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg1, 0, 0, UINT64_C(0x3FF0000000000000)));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg2, 0, 0, UINT64_C(0x4000000000000000)));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg3, 0, 0, UINT64_C(0x4008000000000000)));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_FNMADD, reg4, reg1, reg2, reg3));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xEB,0x1A,                          // jmp 0x20
    0x00,0x00,0x00,0x00,0x00,0x00,0x00, // (padding)
    0x00,0x00,0x00,                     // (padding)

    0x00,0x00,0x00,0x00,                // (data)
    0x00,0x00,0x00,0x80,                // (data)
    0x00,0x00,0x00,0x00,                // (data)
    0x00,0x00,0x00,0x00,                // (data)

    0x48,0xB8,0x00,0x00,0x00,0x00,0x00, // mov $0x3FF0000000000000,%rax
      0x00,0xF0,0x3F,
    0x66,0x48,0x0F,0x6E,0xC8,           // movq %rax,%xmm1
    0x48,0xB8,0x00,0x00,0x00,0x00,0x00, // mov $0x4000000000000000,%rax
      0x00,0x00,0x40,
    0x66,0x48,0x0F,0x6E,0xD0,           // movq %rax,%xmm2
    0x48,0xB8,0x00,0x00,0x00,0x00,0x00, // mov $0x4008000000000000,%rax
      0x00,0x08,0x40,
    0x66,0x48,0x0F,0x6E,0xD8,           // movq %rax,%xmm3
    0x0F,0x28,0xE1,                     // movaps %xmm1,%xmm4
    0xF2,0x0F,0x59,0xE2,                // mulsd %xmm2,%xmm4
    0x0F,0x57,0x25,0xB5,0xFF,0xFF,0xFF, // xorps -75(%rip),%xmm4
    0xF2,0x0F,0x58,0xE3,                // addsd %xmm3,%xmm4
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
