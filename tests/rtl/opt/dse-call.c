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


static unsigned int opt_flags = BINREC_OPT_DSE;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x123456789));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, reg2, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    "    0: LOAD_IMM   r1, 0x123456789\n"
    "    1: CALL       r2, @r1\n"
    "\n"
    "Block 0: <none> --> [0,1] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
