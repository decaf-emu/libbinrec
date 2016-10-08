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
    int label1, label2, reg1, reg2;
    EXPECT(label1 = rtl_alloc_label(unit));
    EXPECT(label2 = rtl_alloc_label(unit));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg1, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg2, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Threading branch at 2 through to final target L1\n"
        "[info] Threading branch at 3 through to final target L1\n"
        "[info] Dropping dead block 3 (6-7)\n"
        "[info] Dropping branch at 3 to next insn\n"
        "[info] r2 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GOTO_IF_Z  r1, L1\n"
    "    3: NOP\n"
    "    4: LABEL      L1\n"
    "    5: RETURN\n"
    "    6: LABEL      L2\n"
    "    7: GOTO       L1\n"
    "\n"
    "Block 0: <none> --> [0,2] --> 1,2\n"
    "Block 1: 0 --> [3,3] --> 2\n"
    "Block 2: 1,0 --> [4,5] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
