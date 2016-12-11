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
    alloc_dummy_registers(unit, 1, RTLTYPE_INT32);

    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ZCAST, reg3, reg2, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg4, reg1, reg3, 0));
    /* This should copy the value to a temporary register before bswapping
     * it so the original value can be used as an index register. */
    EXPECT(rtl_add_insn(unit, RTLOP_STORE_I16_BR, 0, reg4, reg2, 0));
    /* The temporary should not be bswapped back just because reg2 is
     * still live. */
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x8B,0xCE,                          // mov %esi,%ecx
    0x0F,0xC9,                          // bswap %ecx
    0xC1,0xE9,0x10,                     // shr $16,%ecx
    0x66,0x89,0x0C,0x37,                // mov %cx,(%rdi,%rsi)
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 4\n"
        "[info] r4 no longer used, setting death = birth\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] Killing instruction 3\n"
        "[info] Extending r2 live range to 5\n"
    #endif
    "";

#include "tests/rtl-translate-test.i"
