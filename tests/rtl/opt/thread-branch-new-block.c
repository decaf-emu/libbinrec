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
    int label1, label2;
    EXPECT(label1 = rtl_alloc_label(unit));
    EXPECT(label2 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Threading branch at 0 through to final target L1\n"
        "[info] Dropping dead block 8 (10-10)\n"
        "[info] Killing instruction 10\n"
        "[info] Dropping dead block 7 (9-9)\n"
        "[info] Killing instruction 9\n"
        "[info] Dropping dead block 6 (8-8)\n"
        "[info] Killing instruction 8\n"
        "[info] Dropping dead block 5 (7-7)\n"
        "[info] Killing instruction 7\n"
        "[info] Dropping dead block 4 (6-6)\n"
        "[info] Killing instruction 6\n"
        "[info] Dropping dead block 3 (5-5)\n"
        "[info] Killing instruction 5\n"
        "[info] Dropping dead block 2 (3-4)\n"
        "[info] Killing instruction 4\n"
        "[info] Killing instruction 3\n"
        "[info] Dropping branch at 0 to next insn\n"
        "[info] Killing instruction 0\n"
        "[info] Dropping unused label L1\n"
    #endif
    "    0: NOP\n"
    "    1: NOP\n"
    "    2: RETURN\n"
    "    3: NOP\n"
    "    4: NOP\n"
    "    5: NOP\n"
    "    6: NOP\n"
    "    7: NOP\n"
    "    8: NOP\n"
    "    9: NOP\n"
    "   10: NOP\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,2] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
