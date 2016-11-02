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
    .host_features = BINREC_FEATURE_X86_MOVBE,
};
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    int dummy_gpr[13];
    for (int i = 0; i < lenof(dummy_gpr); i++) {
        EXPECT(dummy_gpr[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_gpr[i], 0, 0, 0));
    }
    int dummy_xmm[14];
    for (int i = 0; i < lenof(dummy_xmm); i++) {
        EXPECT(dummy_xmm[i] = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_xmm[i], 0, 0, 0));
    }

    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    /* Use reg3 to spill reg2 and leave a register (other than R15)
     * available for the reg4 load, so we can check that R15 is properly
     * touched by STORE_BR. */
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 0x40800000));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 5));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE_BR, 0, reg2, reg1, 0));
    for (int i = 0; i < lenof(dummy_gpr); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_gpr[i], 0, 0));
    }
    for (int i = 0; i < lenof(dummy_xmm); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_xmm[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg4, reg5, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x56,                               // push %rsi
    0x57,                               // push %rdi
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x41,0x57,                          // push %r15
    0x48,0x81,0xEC,0xB8,0x00,0x00,0x00, // sub $184,%rsp
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
    0x44,0x0F,0x29,0xBC,0x24,0xA0,0x00, // movaps %xmm15,160(%rsp)
      0x00,0x00,
    0x41,0xBE,0x00,0x00,0x80,0x3F,      // mov $0x3F800000,%r14d
    0x66,0x45,0x0F,0x6E,0xF6,           // movd %r14d,%xmm14
    0x41,0xBE,0x02,0x00,0x00,0x00,      // mov $2,%r14d
    0x4C,0x89,0x34,0x24,                // mov %r14,(%rsp)
    0x41,0xBE,0x03,0x00,0x00,0x00,      // mov $3,%r14d
    0xF3,0x44,0x0F,0x11,0x74,0x24,0x08, // movss %xmm14,8(%rsp)
    0x41,0xBE,0x00,0x00,0x80,0x40,      // mov $0x40800000,%r14d
    0x66,0x45,0x0F,0x6E,0xF6,           // movd %r14d,%xmm14
    0x41,0xBE,0x05,0x00,0x00,0x00,      // mov $5,%r14d
    0x4C,0x8B,0x3C,0x24,                // mov (%rsp),%r15
    0x66,0x4C,0x0F,0x6E,0xF8,           // movq %rax,%xmm15
    0x8B,0x44,0x24,0x08,                // mov 8(%rsp),%eax
    0x41,0x0F,0x38,0xF1,0x07,           // movbe %eax,(%r15)
    0x66,0x4C,0x0F,0x7E,0xF8,           // movq %xmm15,%rax
    0x44,0x0F,0x28,0xBC,0x24,0xA0,0x00, // movaps 160(%rsp),%xmm15
      0x00,0x00,
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
    0x48,0x81,0xC4,0xB8,0x00,0x00,0x00, // add $184,%rsp
    0x41,0x5F,                          // pop %r15
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5F,                               // pop %rdi
    0x5E,                               // pop %rsi
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
