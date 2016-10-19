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
    alloc_dummy_registers(unit, 1, RTLTYPE_INT32);

    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_BFINS, reg3, reg1, reg2, 2 | 32<<8));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB9,0x01,0x00,0x00,0x00,           // mov $1,%ecx
    0xBA,0x02,0x00,0x00,0x00,           // mov $2,%edx
    0x48,0xBE,0x03,0x00,0x00,0x00,0xFC, // mov $0xFFFFFFFC00000003,%rsi
      0xFF,0xFF,0xFF,
    0x48,0x23,0xCE,                     // and %rsi,%rcx
    0x8B,0xF2,                          // mov %edx,%esi
    0x48,0xC1,0xE6,0x02,                // shl $2,%rsi
    0x48,0x0B,0xCE,                     // or %rsi,%rcx
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
