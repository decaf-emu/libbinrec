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
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));

    /* Append a GOTO instruction that's past the end of the array to make
     * sure the scanner stays within the instruction limit. */
    ASSERT(unit->insns_size > unit->num_insns);
    unit->insns[unit->num_insns].opcode = RTLOP_GOTO;
    unit->insns[unit->num_insns].label = label1;

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead block 1 (1-2)\n"
        "[info] Dropping branch at 0 to next insn\n"
    #endif
    "    0: NOP\n"
    "    1: LABEL      L1\n"
    "    2: RETURN\n"
    "    3: LABEL      L2\n"
    "    4: NOP\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 2\n"
    "Block 2: 0 --> [3,4] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
