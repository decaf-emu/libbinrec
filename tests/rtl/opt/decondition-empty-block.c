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


static unsigned int opt_flags = BINREC_OPT_DECONDITION;

static int add_rtl(RTLUnit *unit)
{
    int label, reg1;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: GOTO       L1\n"
    "    2: GOTO       L1\n"
    "    3: GOTO       L1\n"
    "    4: GOTO       L1\n"
    "    5: GOTO       L1\n"
    "    6: GOTO       L1\n"
    "    7: GOTO       L1\n"
    "    8: GOTO       L1\n"
    "    9: LABEL      L1\n"
    "   10: RETURN\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 8\n"
    "Block 1: <none> --> [2,2] --> 8\n"
    "Block 2: <none> --> [3,3] --> 8\n"
    "Block 3: <none> --> [4,4] --> 8\n"
    "Block 4: <none> --> [5,5] --> 8\n"
    "Block 5: <none> --> [6,6] --> 8\n"
    "Block 6: <none> --> [7,7] --> 8\n"
    "Block 7: <none> --> [8,8] --> 8\n"
    "Block 8: 0,7,1,2,3,4,5,6 --> [9,10] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
