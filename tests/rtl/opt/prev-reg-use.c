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


static unsigned int opt_flags = 0;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x1234));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg2, reg1, 0, 0));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg3, reg2, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg4, reg1, 0, 0));

    /* rtl_opt_prev_reg_use() is only called externally (local functions
     * all call the inline version), so call it once here to verify that
     * it works as advertised. */
    EXPECT_EQ(rtl_opt_prev_reg_use(unit, reg1, 3), 1);

    return EXIT_SUCCESS;
}

static const char expected[] =
    "    0: LOAD_IMM   r1, 0x1234\n"
    "    1: LOAD       r2, 0(r1)\n"
    "    2: MOVE       r3, r2\n"
    "    3: MOVE       r4, r1\n"
    "\n"
    "Block 0: <none> --> [0,3] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
