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
    int dummy_regs[13];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }
    alloc_dummy_registers(unit, 3, RTLTYPE_FLOAT32);

    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE_BR, 0, reg2, reg1, 0));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));

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
    0x48,0x83,0xEC,0x28,                // sub $40,%rsp
    0x44,0x0F,0x29,0x7C,0x24,0x10,      // movaps %xmm15,16(%rsp)
    0x41,0xBE,0x00,0x00,0x80,0x3F,      // mov $0x3F800000,%r14d
    0x66,0x41,0x0F,0x6E,0xDE,           // movd %r14d,%xmm3
    0x41,0xBE,0x02,0x00,0x00,0x00,      // mov $2,%r14d
    0x4C,0x89,0x34,0x24,                // mov %r14,(%rsp)
    0x41,0xBE,0x03,0x00,0x00,0x00,      // mov $3,%r14d
    0x4C,0x8B,0x3C,0x24,                // mov (%rsp),%r15
    0x66,0x4C,0x0F,0x6E,0xF8,           // movq %rax,%xmm15
    0x66,0x0F,0x7E,0xD8,                // movd %xmm3,%eax
    0x41,0x0F,0x38,0xF1,0x07,           // movbe %eax,(%r15)
    0x66,0x4C,0x0F,0x7E,0xF8,           // movq %xmm15,%rax
    0x44,0x0F,0x28,0x7C,0x24,0x10,      // movaps 16(%rsp),%xmm15
    0x48,0x83,0xC4,0x28,                // add $40,%rsp
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
