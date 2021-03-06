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
    alloc_dummy_registers(unit, 5, RTLTYPE_INT32);

    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_DIVU, reg3, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x41,0xB8,0x01,0x00,0x00,0x00,      // mov $1,%r8d
    0x41,0xB9,0x02,0x00,0x00,0x00,      // mov $2,%r9d
    0x4C,0x8B,0xDA,                     // mov %rdx,%r11
    0x4C,0x8B,0xD0,                     // mov %rax,%r10
    0x49,0x8B,0xC0,                     // mov %r8,%rax
    0x33,0xD2,                          // xor %edx,%edx
    0x49,0xF7,0xF1,                     // div %r9
    0x49,0x87,0xC2,                     // xchg %rax,%r10
    0x49,0x8B,0xD3,                     // mov %r11,%rdx
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
