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
    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg4, reg1, reg2, reg3));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Reduced r4 SELECT (always true) to MOVE from r1 at 3\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] r3 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_ARG   r1, 1\n"
    "    1: LOAD_ARG   r2, 2\n"
    "    2: LOAD_IMM   r3, 3\n"
    "    3: MOVE       r4, r1\n"
    "\n"
    "Block 0: <none> --> [0,3] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
