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
    int label, reg1;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg1, 0, label));

    /* We should normally never get an empty block in the code stream,
     * but check behavior with an empty block just in case.  Note that
     * the empty block will prevent elimination of the branch, but since
     * this is an impossible case anyway, we don't worry about it. */
    EXPECT(rtl_block_add(unit));
    unit->cur_block = unit->last_block;
    unit->blocks[unit->cur_block].first_insn = unit->num_insns;
    EXPECT(rtl_block_add_edge(unit, unit->cur_block - 1, unit->cur_block));

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: GOTO_IF_Z  r1, L1\n"
    "    2: LABEL      L1\n"
    "    3: RETURN\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1,2\n"
    "Block 1: 0 --> [empty] --> 2\n"
    "Block 2: 1,0 --> [2,3] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
