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


static unsigned int opt_flags = BINREC_OPT_DSE
                              | BINREC_OPT_FOLD_CONSTANTS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg3, reg2, reg2, reg2));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg1, reg3, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Reduced r3 SELECT (identical values) to MOVE from r2 at 2\n"
        "[info] Folded r3 to constant value 1 at 2\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] Dropping dead store to r2 at 1\n"
        "[info] Killing instruction 1\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: NOP\n"
    "    2: LOAD_IMM   r3, 1\n"
    "    3: STORE      0(r1), r3\n"
    "\n"
    "Block 0: <none> --> [0,3] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
