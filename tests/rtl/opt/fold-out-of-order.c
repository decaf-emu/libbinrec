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
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg3, reg4, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg2, reg3, 0, 2));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg1, reg3, reg2, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Folded r3 to constant value 1 at 1\n"
        "[info] r4 no longer used, setting death = birth\n"
        "[info] Folded r2 to constant value 3 at 2\n"
        "[info] Folded r1 to constant value 4 at 3\n"
        "[info] r3 no longer used, setting death = birth\n"
        "[info] r2 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_IMM   r4, 1\n"
    "    1: LOAD_IMM   r3, 1\n"
    "    2: LOAD_IMM   r2, 3\n"
    "    3: LOAD_IMM   r1, 4\n"
    "\n"
    "Block 0: <none> --> [0,3] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
