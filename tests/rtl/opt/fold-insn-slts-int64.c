/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/rtl-internal.h"
#include "tests/common.h"


static unsigned int opt_flags = BINREC_OPT_FOLD_CONSTANTS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, reg5, reg6, reg7;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg1, 0, 0, UINT64_C(0x123456789)));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg2, 0, 0, UINT64_C(0x102030405)));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg3, 0, 0, UINT64_C(0x123456789)));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg4, 0, 0, UINT64_C(-0x102030405)));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_SLTS, reg5, reg1, reg2, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_SLTS, reg6, reg1, reg3, 0));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_SLTS, reg7, reg1, reg4, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Folded r5 to constant value 0x0 at insn 4\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] Folded r6 to constant value 0x0 at insn 5\n"
        "[info] r3 no longer used, setting death = birth\n"
        "[info] Folded r7 to constant value 0x0 at insn 6\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] r4 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_IMM   r1, 0x123456789\n"
    "    1: LOAD_IMM   r2, 0x102030405\n"
    "    2: LOAD_IMM   r3, 0x123456789\n"
    "    3: LOAD_IMM   r4, 0xFFFFFFFEFDFCFBFB\n"
    "    4: LOAD_IMM   r5, 0x0\n"
    "    5: LOAD_IMM   r6, 0x0\n"
    "    6: LOAD_IMM   r7, 0x0\n"
    "\n"
    "Block 0: <none> --> [0,6] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
