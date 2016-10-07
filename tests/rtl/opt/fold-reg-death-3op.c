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
    int reg1, reg2, reg3, reg4, reg5, reg6, reg7;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x1234));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x5678));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0x9ABC));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_CMPXCHG, reg4, reg1, reg2, reg3));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg5, reg1, reg2, reg4));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg6, reg1, reg2, 0));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg7, reg6, reg3, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Folded r6 to constant value 0x68AC at insn 5\n"
        "[info] r1 still used at insn 4\n"
        "[info] r2 still used at insn 4\n"
        "[info] Folded r7 to constant value 0x10368 at insn 6\n"
        "[info] r6 no longer used, setting death = birth\n"
        "[info] r3 still used at insn 3\n"
    #endif
    "    0: LOAD_IMM   r1, 0x1234\n"
    "    1: LOAD_IMM   r2, 0x5678\n"
    "    2: LOAD_IMM   r3, 0x9ABC\n"
    "    3: CMPXCHG    r4, (r1), r2, r3\n"
    "    4: SELECT     r5, r1, r2, r4\n"
    "    5: LOAD_IMM   r6, 0x68AC\n"
    "    6: LOAD_IMM   r7, 0x10368\n"
    "\n"
    "Block 0: <none> --> [0,6] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
