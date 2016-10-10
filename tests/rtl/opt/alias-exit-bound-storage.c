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
    int label, alias, reg1, reg2;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 0));
    rtl_set_alias_storage(unit, alias, reg2, 0);
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg1, 0, alias));

    return EXIT_SUCCESS;
}

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 0\n"
    "    2: SET_ALIAS  a1, r1\n"
    "\n"
    "Alias 1: int32 @ 0(r2)\n"
    "\n"
    "Block 0: <none> --> [0,2] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
