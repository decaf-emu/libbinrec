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
    int reg1, chain_insn;
    EXPECT_EQ(chain_insn = rtl_add_chain_insn(unit, 0, 0), 0);
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_CHAIN_RESOLVE, 0, reg1, 0, chain_insn));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x41,0x57,                          // push %r15
    0x66,0x0F,0x1F,0x44,0x00,0x00,      // nopw 0(%rax,%rax,1)
    0xE9,0x0C,0x00,0x00,0x00,           // L1: jmp L2
    0x00,0x00,0x00,0x00,0x00,           // (data)
    0x49,0x8B,0xC7,                     // mov %r15,%rax
    0x41,0x5F,                          // pop %r15
    0xFF,0xE0,                          // jmp *%rax
    0xB8,0x01,0x00,0x00,0x00,           // L2: mov $1,%eax
    0x48,0x85,0xC0,                     // test %rax,%rax
    0x74,0x25,                          // jz L4
    0x4C,0x8B,0xF8,                     // mov %rax,%r15
    0x49,0xC1,0xEF,0x30,                // shr $48,%r15
    0x74,0x08,                          // jz L3
    0x66,0x44,0x89,0x3D,0xDC,0xFF,0xFF, // mov %r15w,-36(%rip)
      0xFF,
    0x48,0xC1,0xE0,0x10,                // L3: shl $16,%rax
    0x48,0x81,0xC8,0x49,0xBF,0x00,0x00, // or $0xBF49,%rax
    0x48,0x89,0x05,0xC2,0xFF,0xFF,0xFF, // mov %rax,-62(%rip)
    0xEB,0xC0,                          // jmp L1
    0x41,0x5F,                          // L4: pop %r15
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
