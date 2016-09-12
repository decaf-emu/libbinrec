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
    alloc_dummy_registers(unit, 4, RTLTYPE_INT32);

    uint32_t reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_CLZ, reg2, reg1, 0, 0));

    alloc_dummy_registers(unit, 2, RTLTYPE_INT32);

    uint32_t reg3, reg4;
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_CLZ, reg4, reg3, 0, 0));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x33,0xFF,                          // xor %edi,%edi
    0x4C,0x0F,0xBD,0xC7,                // bsr %rdi,%r8
    0x41,0xB9,0x20,0x00,0x00,0x00,      // mov $32,%r9d
    0x4D,0x0F,0x44,0xC1,                // cmovz %r9,%r8
    0x45,0x33,0xDB,                     // xor %r11d,%r11d
    0x49,0x0F,0xBD,0xDB,                // bsr %r11,%rbx
    0xBD,0x20,0x00,0x00,0x00,           // mov $32,%ebp
    0x48,0x0F,0x44,0xDD,                // cmovz %rbp,%rbx
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"