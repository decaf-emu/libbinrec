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
    int label, alias, reg1, reg2, reg3, reg4, reg5;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg1, 0, alias));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg3, 0, 0, alias));
    /* This SET_ALIAS will fail to be eliminated if the code blindly
     * assumes that a GET_ALIAS prevents killing any SET_ALIASes in the
     * same block. */
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg4, reg2, reg3, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg4, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg5, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, reg5, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead store to a1 at 4\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: SET_ALIAS  a1, r1\n"
    "    3: GET_ALIAS  r3, a1\n"
    "    4: NOP\n"
    "    5: ADD        r4, r2, r3\n"
    "    6: SET_ALIAS  a1, r4\n"
    "    7: LABEL      L1\n"
    "    8: GET_ALIAS  r5, a1\n"
    "    9: RETURN     r5\n"
    "\n"
    "Alias 1: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 1\n"
    "Block 1: 0 --> [7,9] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
