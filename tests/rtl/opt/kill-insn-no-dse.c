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
    int reg1, reg2, reg3, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));

    int reg4;
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg4, reg1, reg2, reg3));

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead block 1 (4-4)\n"
        "[info] Killing instruction 4\n"
        "[info] r3 no longer used, setting death = birth\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] Dropping branch at 3 to next insn\n"
        "[info] Killing instruction 3\n"
        "[info] Dropping unused label L1\n"
    #endif
    "    0: LOAD_IMM   r1, 1\n"
    "    1: LOAD_IMM   r2, 2\n"
    "    2: LOAD_IMM   r3, 3\n"
    "    3: NOP\n"
    "    4: NOP\n"
    "    5: NOP\n"
    "\n"
    "Block 0: <none> --> [0,3] --> 2\n"
    "Block 2: 0 --> [5,5] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
