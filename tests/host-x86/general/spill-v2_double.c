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
    uint32_t base, regs[17];
    EXPECT(base = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, base, 0, 0, 0));
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(regs[i] = rtl_alloc_register(unit, RTLTYPE_V2_DOUBLE));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD, regs[i], base, 0, i*16));
    }
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, regs[i], 0, 0));
    }

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x28,                // sub $40,%rsp
    0x0F,0x28,0x07,                     // movaps (%rdi),%xmm0
    0x0F,0x28,0x4F,0x10,                // movaps 16(%rdi),%xmm1
    0x0F,0x28,0x57,0x20,                // movaps 32(%rdi),%xmm2
    0x0F,0x28,0x5F,0x30,                // movaps 48(%rdi),%xmm3
    0x0F,0x28,0x67,0x40,                // movaps 64(%rdi),%xmm4
    0x0F,0x28,0x6F,0x50,                // movaps 80(%rdi),%xmm5
    0x0F,0x28,0x77,0x60,                // movaps 96(%rdi),%xmm6
    0x0F,0x28,0x7F,0x70,                // movaps 112(%rdi),%xmm7
    0x44,0x0F,0x28,0x87,0x80,0x00,0x00, // movaps 128(%rdi),%xmm8
      0x00,
    0x44,0x0F,0x28,0x8F,0x90,0x00,0x00, // movaps 144(%rdi),%xmm9
      0x00,
    0x44,0x0F,0x28,0x97,0xA0,0x00,0x00, // movaps 160(%rdi),%xmm10
      0x00,
    0x44,0x0F,0x28,0x9F,0xB0,0x00,0x00, // movaps 176(%rdi),%xmm11
      0x00,
    0x44,0x0F,0x28,0xA7,0xC0,0x00,0x00, // movaps 192(%rdi),%xmm12
      0x00,
    0x44,0x0F,0x28,0xAF,0xD0,0x00,0x00, // movaps 208(%rdi),%xmm13
      0x00,
    0x44,0x0F,0x28,0xB7,0xE0,0x00,0x00, // movaps 224(%rdi),%xmm14
      0x00,
    0x44,0x0F,0x29,0x34,0x24,           // movaps %xmm14,(%rsp)
    0x44,0x0F,0x28,0xB7,0xF0,0x00,0x00, // movaps 240(%rdi),%xmm14
      0x00,
    0x44,0x0F,0x29,0x74,0x24,0x10,      // movaps %xmm14,16(%rsp)
    0x44,0x0F,0x28,0xB7,0x00,0x01,0x00, // movaps 256(%rdi),%xmm14
      0x00,
    0x48,0x83,0xC4,0x28,                // add $40,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
