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
    int reg1, reg2, reg3, chain_insn;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT_EQ(chain_insn = rtl_add_chain_insn(unit, reg1, reg2), 2);
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    EXPECT(rtl_add_insn(unit, RTLOP_CHAIN_RESOLVE, 0, reg3, 0, chain_insn));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x41,0x57,                          // push %r15
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0xB9,0x02,0x00,0x00,0x00,           // mov $2,%ecx
    0x0F,0x1F,0x40,0x00,                // nopl 0(%rax)
    0xE9,0x12,0x00,0x00,0x00,           // jmp L0
    0x00,0x00,0x00,0x00,0x00,           // (data)
    0x48,0x8B,0xF8,                     // mov %rax,%rdi
    0x48,0x8B,0xF1,                     // mov %rcx,%rsi
    0x49,0x8B,0xC7,                     // mov %r15,%rax
    0x41,0x5F,                          // pop %r15
    0xFF,0xE0,                          // jmp *%rax
    0xB8,0x03,0x00,0x00,0x00,           // L0: mov $3,%eax
    0x48,0x85,0xC0,                     // test %rax,%rax
    0x74,0x22,                          // jz L1
    0x48,0x8B,0xC8,                     // mov %rax,%rcx
    0x48,0xC1,0xE9,0x30,                // shr $48,%rcx
    0x74,0x07,                          // jz L2
    0x66,0x89,0x0D,0xD7,0xFF,0xFF,0xFF, // mov %cx,-41(%rip)
    0x48,0xC1,0xE0,0x10,                // shl $16,%rax
    0x48,0x81,0xC8,0x49,0xBF,0x00,0x00, // or $0xBF49,%rax
    0x48,0x89,0x05,0xBD,0xFF,0xFF,0xFF, // mov %rax,-67(%rip)
    0x41,0x5F,                          // L1: pop %r15
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
