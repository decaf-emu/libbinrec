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
static const unsigned int host_opt = BINREC_OPT_H_X86_ADDRESS_OPERANDS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 1));
    alloc_dummy_registers(unit, 3, RTLTYPE_INT32);
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ORI, reg3, reg2, 0, 1));  // Gets R8.
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg4, reg1, reg3, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg5, reg4, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x4C,0x8B,0xC6,                     // mov %rsi,%r8
    0x49,0x83,0xC8,0x01,                // or $1,%r8
    0x42,0x8B,0x3C,0x07,                // mov (%rdi,%r8),%edi
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 7\n"
        "[info] r6 no longer used, setting death = birth\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] Extending r1 live range to 8\n"
        "[info] Extending r6 live range to 8\n"
    #endif
    "";

#include "tests/rtl-translate-test.i"
