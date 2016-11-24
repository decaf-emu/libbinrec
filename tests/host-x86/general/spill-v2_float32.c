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
    int base, regs[17];
    EXPECT(base = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, base, 0, 0, 0));
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(regs[i] = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD, regs[i], base, 0, i*8));
    }
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, regs[i], 0, 0));
    }

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x28,                // sub $40,%rsp
    0xF2,0x0F,0x10,0x07,                // movsd (%rdi),%xmm0
    0xF2,0x0F,0x10,0x4F,0x08,           // movsd 8(%rdi),%xmm1
    0xF2,0x0F,0x10,0x57,0x10,           // movsd 16(%rdi),%xmm2
    0xF2,0x0F,0x10,0x5F,0x18,           // movsd 24(%rdi),%xmm3
    0xF2,0x0F,0x10,0x67,0x20,           // movsd 32(%rdi),%xmm4
    0xF2,0x0F,0x10,0x6F,0x28,           // movsd 40(%rdi),%xmm5
    0xF2,0x0F,0x10,0x77,0x30,           // movsd 48(%rdi),%xmm6
    0xF2,0x0F,0x10,0x7F,0x38,           // movsd 56(%rdi),%xmm7
    0xF2,0x44,0x0F,0x10,0x47,0x40,      // movsd 64(%rdi),%xmm8
    0xF2,0x44,0x0F,0x10,0x4F,0x48,      // movsd 72(%rdi),%xmm9
    0xF2,0x44,0x0F,0x10,0x57,0x50,      // movsd 80(%rdi),%xmm10
    0xF2,0x44,0x0F,0x10,0x5F,0x58,      // movsd 88(%rdi),%xmm11
    0xF2,0x44,0x0F,0x10,0x67,0x60,      // movsd 96(%rdi),%xmm12
    0xF2,0x44,0x0F,0x10,0x6F,0x68,      // movsd 104(%rdi),%xmm13
    0xF2,0x44,0x0F,0x10,0x77,0x70,      // movsd 112(%rdi),%xmm14
    0x44,0x0F,0x29,0x34,0x24,           // movaps %xmm14,(%rsp)
    0xF2,0x44,0x0F,0x10,0x77,0x78,      // movsd 120(%rdi),%xmm14
    0x44,0x0F,0x29,0x74,0x24,0x10,      // movaps %xmm14,16(%rsp)
    0xF2,0x44,0x0F,0x10,0xB7,0x80,0x00, // movsd 128(%rdi),%xmm14
      0x00,0x00,
    0x48,0x83,0xC4,0x28,                // add $40,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
