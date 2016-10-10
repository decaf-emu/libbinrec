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
    int label, alias, reg1, reg2, reg3;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg1, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg2, 0, 0, alias));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg3, reg2, 0, -1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg3, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: SET_ALIAS  a1, r1\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r2, a1\n"
    "    4: ADDI       r3, r2, -1\n"
    "    5: SET_ALIAS  a1, r3\n"
    "    6: GOTO_IF_Z  r3, L1\n"
    "    7: RETURN\n"
    "\n"
    "Alias 1: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0,1 --> [2,6] --> 2,1\n"
    "Block 2: 1 --> [7,7] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
