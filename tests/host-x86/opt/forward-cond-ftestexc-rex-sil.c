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
static const unsigned int host_opt = BINREC_OPT_H_X86_FORWARD_CONDITIONS;

static int add_rtl(RTLUnit *unit)
{
    alloc_dummy_registers(unit, 3, RTLTYPE_INT32);

    int label, reg1, reg2;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(rtl_add_insn(unit, RTLOP_FGETSTATE, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC, reg2, reg1, 0, RTLFEXC_INVALID));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg2, 0, label));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x0F,0xAE,0x1C,0x24,                // L1: stmxcsr (%rsp)
    0x8B,0x34,0x24,                     // mov (%rsp),%esi
    0x40,0xF6,0xC6,0x01,                // test $1,%sil
    0x75,0xF3,                          // jnz L1
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Extending r4 live range to 6\n"
        "[info] Killing instruction 5\n"
    #endif
    "";

#include "tests/rtl-translate-test.i"
