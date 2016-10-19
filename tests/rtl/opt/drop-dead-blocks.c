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


static unsigned int opt_flags = BINREC_OPT_BASIC;

static int add_rtl(RTLUnit *unit)
{
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead block 1 (1-1)\n"
        "[info] Killing instruction 1\n"
    #endif
    "    0: RETURN\n"
    "    1: NOP\n"
    "\n"
    "Block 0: <none> --> [0,0] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
