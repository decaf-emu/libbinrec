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

static int add_rtl(RTLUnit *unit)
{
    alloc_dummy_registers(unit, 3, RTLTYPE_INT32);

    uint32_t reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_BFINS, reg3, reg1, reg2, 2 | 8<<8));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x33,0xF6,                          // xor %esi,%esi
    0x33,0xFF,                          // xor %edi,%edi
    0x44,0x8B,0xC6,                     // mov %esi,%r8d
    0x41,0x81,0xE0,0x03,0xFC,0xFF,0xFF, // and $0xFFFFFC03,%r8d
    /* Make sure %dil doesn't trigger an additional empty REX prefix. */
    0x44,0x0F,0xB6,0xCF,                // movzbl %dil,%r9d
    0x41,0xC1,0xE1,0x02,                // shl $2,%r9d
    0x45,0x0B,0xC1,                     // or %r9d,%r8d
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
