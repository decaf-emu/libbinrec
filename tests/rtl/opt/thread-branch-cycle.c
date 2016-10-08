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


static unsigned int opt_flags = BINREC_OPT_BASIC;

static int add_rtl(RTLUnit *unit)
{
    int label1, label2;
    EXPECT(label1 = rtl_alloc_label(unit));
    EXPECT(label2 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Branch at 0 has a cycle, not threading\n"
        "[info] Branch at 2 has a cycle, not threading\n"
        "[info] Branch at 4 has a cycle, not threading\n"
    #endif
    "    0: GOTO       L2\n"
    "    1: LABEL      L1\n"
    "    2: GOTO       L1\n"
    "    3: LABEL      L2\n"
    "    4: GOTO       L1\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 2\n"
    "Block 1: 1,2 --> [1,2] --> 1\n"
    "Block 2: 0 --> [3,4] --> 1\n"
    ;

#include "tests/rtl-optimize-test.i"
