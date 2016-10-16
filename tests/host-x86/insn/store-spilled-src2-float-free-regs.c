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
    .host = BINREC_ARCH_X86_64_WINDOWS,  // To check whether XMM15 is pushed.
};
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    int dummy_regs[14];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40000000));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg3, reg1, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x81,0xEC,0xA8,0x00,0x00,0x00, // sub $168,%rsp
    0x0F,0x29,0x74,0x24,0x10,           // movaps %xmm6,16(%rsp)
    0x0F,0x29,0x7C,0x24,0x20,           // movaps %xmm7,32(%rsp)
    0x44,0x0F,0x29,0x44,0x24,0x30,      // movaps %xmm8,48(%rsp)
    0x44,0x0F,0x29,0x4C,0x24,0x40,      // movaps %xmm9,64(%rsp)
    0x44,0x0F,0x29,0x54,0x24,0x50,      // movaps %xmm10,80(%rsp)
    0x44,0x0F,0x29,0x5C,0x24,0x60,      // movaps %xmm11,96(%rsp)
    0x44,0x0F,0x29,0x64,0x24,0x70,      // movaps %xmm12,112(%rsp)
    0x44,0x0F,0x29,0xAC,0x24,0x80,0x00, // movaps %xmm13,128(%rsp)
      0x00,0x00,
    0x44,0x0F,0x29,0xB4,0x24,0x90,0x00, // movaps %xmm14,144(%rsp)
      0x00,0x00,
    0xB8,0x00,0x00,0x80,0x3F,           // mov $0x3F800000,%eax
    0x66,0x44,0x0F,0x6E,0xF0,           // movd %eax,%xmm14
    0xF3,0x44,0x0F,0x11,0x34,0x24,      // movss %xmm14,(%rsp)
    0xB8,0x00,0x00,0x00,0x40,           // mov $0x40000000,%eax
    0x66,0x44,0x0F,0x6E,0xF0,           // movd %eax,%xmm14
    0xB8,0x03,0x00,0x00,0x00,           // mov $3,%eax
    0xF3,0x0F,0x10,0x04,0x24,           // movss (%rsp),%xmm0
    0xF3,0x0F,0x11,0x00,                // movss %xmm0,(%rax)
    0x44,0x0F,0x28,0xB4,0x24,0x90,0x00, // movaps 144(%rsp),%xmm14
      0x00,0x00,
    0x44,0x0F,0x28,0xAC,0x24,0x80,0x00, // movaps 128(%rsp),%xmm13
      0x00,0x00,
    0x44,0x0F,0x28,0x64,0x24,0x70,      // movaps 112(%rsp),%xmm12
    0x44,0x0F,0x28,0x5C,0x24,0x60,      // movaps 96(%rsp),%xmm11
    0x44,0x0F,0x28,0x54,0x24,0x50,      // movaps 80(%rsp),%xmm10
    0x44,0x0F,0x28,0x4C,0x24,0x40,      // movaps 64(%rsp),%xmm9
    0x44,0x0F,0x28,0x44,0x24,0x30,      // movaps 48(%rsp),%xmm8
    0x0F,0x28,0x7C,0x24,0x20,           // movaps 32(%rsp),%xmm7
    0x0F,0x28,0x74,0x24,0x10,           // movaps 16(%rsp),%xmm6
    0x48,0x81,0xC4,0xA8,0x00,0x00,0x00, // add $168,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
