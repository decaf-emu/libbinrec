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
    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 1234));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 5678));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, reg4, reg1, reg2, reg3));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg5, reg3, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg4, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead store to r5 at 4\n"
        "[info] Killing instruction 4\n"
        "[info] r3 death rolled back to 3\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 1234\n"
    "    2: LOAD_IMM   r3, 5678\n"
    "    3: CALL       r4, @r1, r2, r3\n"
    "    4: NOP\n"
    "    5: NOP        -, r4\n"
    "\n"
    "Block 0: <none> --> [0,5] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
