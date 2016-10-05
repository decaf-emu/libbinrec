/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/rtl.h"
#include "tests/common.h"


static unsigned int opt_flags = BINREC_OPT_BASIC;

static int add_rtl(RTLUnit *unit)
{
    int label, reg1;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg1, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    "[info] [RTL] Dropping dead block 3 (4-4)\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GOTO_IF_Z  r1, L1\n"
    "    3: GOTO       L1\n"
    "    4: RETURN\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0,1,2 --> [1,2] --> 2,1\n"
    "Block 2: 1 --> [3,3] --> 1\n"
    ;

#include "tests/rtl-optimize-test.i"
