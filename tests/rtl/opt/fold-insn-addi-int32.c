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
    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1234));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg2, reg1, 0, 5678));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Folded r2 to constant value 6912 at 1\n"
        "[info] r1 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_IMM   r1, 1234\n"
    "    1: LOAD_IMM   r2, 6912\n"
    "\n"
    "Block 0: <none> --> [0,1] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
