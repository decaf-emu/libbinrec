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


static unsigned int opt_flags = BINREC_OPT_FOLD_CONSTANTS
                              | BINREC_OPT_FOLD_FP_CONSTANTS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg1, 0, 0, UINT64_C(0x3FF0000000000000)));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_FNEG, reg2, reg1, 0, 0));

    int reg3, reg4;
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg3, 0, 0, UINT64_C(0xBFF0000000000000)));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_FNEG, reg4, reg3, 0, 0));

    int reg5, reg6;
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg5, 0, 0, UINT64_C(0x7FF0000000000001)));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_FNEG, reg6, reg5, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Folded r2 to constant value 0xBFF0000000000000 at 1\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] Folded r4 to constant value 0x3FF0000000000000 at 3\n"
        "[info] r3 no longer used, setting death = birth\n"
        "[info] Folded r6 to constant value 0xFFF0000000000001 at 5\n"
        "[info] r5 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_IMM   r1, 1.0\n"
    "    1: LOAD_IMM   r2, -1.0\n"
    "    2: LOAD_IMM   r3, -1.0\n"
    "    3: LOAD_IMM   r4, 1.0\n"
    "    4: LOAD_IMM   r5, nan(0x1)\n"
    "    5: LOAD_IMM   r6, -nan(0x1)\n"
    "\n"
    "Block 0: <none> --> [0,5] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
