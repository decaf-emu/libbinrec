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

    uint32_t reg1, reg2, reg3, reg4, reg5, reg6;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg1, reg2, 0));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg3, reg4, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg6, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg5, reg6, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, reg4, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg5, reg6, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x33,0xFF,                          // xor %edi,%edi
    0x45,0x33,0xC0,                     // xor %r8d,%r8d
    0x44,0x89,0x07,                     // mov %r8d,(%rdi)
    0x45,0x33,0xC9,                     // xor %r9d,%r9d
    0x45,0x33,0xD2,                     // xor %r10d,%r10d
    0x45,0x89,0x11,                     // mov %r10d,(%r9)
    0x45,0x33,0xDB,                     // xor %r11d,%r11d
    0x33,0xDB,                          // xor %ebx,%ebx
    0x41,0x89,0x1B,                     // mov %ebx,(%r11)
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
