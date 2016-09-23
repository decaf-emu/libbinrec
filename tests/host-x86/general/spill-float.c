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
        EXPECT(regs[i] = rtl_alloc_register(unit, RTLTYPE_FLOAT));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD, regs[i], base, 0, i*4));
    }
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, regs[i], 0, 0));
    }

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xF3,0x0F,0x10,0x07,                // movss (%rdi),%xmm0
    0xF3,0x0F,0x10,0x4F,0x04,           // movss 4(%rdi),%xmm1
    0xF3,0x0F,0x10,0x57,0x08,           // movss 8(%rdi),%xmm2
    0xF3,0x0F,0x10,0x5F,0x0C,           // movss 12(%rdi),%xmm3
    0xF3,0x0F,0x10,0x67,0x10,           // movss 16(%rdi),%xmm4
    0xF3,0x0F,0x10,0x6F,0x14,           // movss 20(%rdi),%xmm5
    0xF3,0x0F,0x10,0x77,0x18,           // movss 24(%rdi),%xmm6
    0xF3,0x0F,0x10,0x7F,0x1C,           // movss 28(%rdi),%xmm7
    0xF3,0x44,0x0F,0x10,0x47,0x20,      // movss 32(%rdi),%xmm8
    0xF3,0x44,0x0F,0x10,0x4F,0x24,      // movss 36(%rdi),%xmm9
    0xF3,0x44,0x0F,0x10,0x57,0x28,      // movss 40(%rdi),%xmm10
    0xF3,0x44,0x0F,0x10,0x5F,0x2C,      // movss 44(%rdi),%xmm11
    0xF3,0x44,0x0F,0x10,0x67,0x30,      // movss 48(%rdi),%xmm12
    0xF3,0x44,0x0F,0x10,0x6F,0x34,      // movss 52(%rdi),%xmm13
    0xF3,0x44,0x0F,0x10,0x77,0x38,      // movss 56(%rdi),%xmm14
    0xF3,0x44,0x0F,0x11,0x34,0x24,      // movss %xmm14,(%rsp)
    0xF3,0x44,0x0F,0x10,0x77,0x3C,      // movss 60(%rdi),%xmm14
    0xF3,0x44,0x0F,0x11,0x74,0x24,0x04, // movss %xmm14,4(%rsp)
    0xF3,0x44,0x0F,0x10,0x77,0x40,      // movss 64(%rdi),%xmm14
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
