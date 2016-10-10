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
    int label1, label2, alias, reg1, reg2;
    EXPECT(label1 = rtl_alloc_label(unit));
    EXPECT(label2 = rtl_alloc_label(unit));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg1, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg1, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label2));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg2, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, reg2, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: SET_ALIAS  a1, r1\n"
    "    2: GOTO_IF_NZ r1, L1\n"
    "    3: RETURN     r1\n"
    "    4: LABEL      L1\n"
    "    5: LABEL      L2\n"
    "    6: GET_ALIAS  r2, a1\n"
    "    7: RETURN     r2\n"
    "\n"
    "Alias 1: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,2] --> 1,2\n"
    "Block 1: 0 --> [3,3] --> <none>\n"
    "Block 2: 0 --> [4,4] --> 3\n"
    "Block 3: 2 --> [5,7] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
