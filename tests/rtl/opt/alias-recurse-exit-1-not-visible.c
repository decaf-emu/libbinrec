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


static unsigned int opt_flags = BINREC_OPT_DEEP_DATA_FLOW;

static int add_rtl(RTLUnit *unit)
{
    int label, alias, reg1;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg1, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg1, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead store to a1 at 1\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: NOP\n"
    "    2: GOTO_IF_NZ r1, L1\n"
    "    3: NOP\n"
    "    4: LABEL      L1\n"
    "    5: RETURN\n"
    "\n"
    "Alias 1: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,2] --> 1,2\n"
    "Block 1: 0 --> [3,3] --> 2\n"
    "Block 2: 1,0 --> [4,5] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
