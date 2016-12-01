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
    int reg1, label1;
    EXPECT(label1 = rtl_alloc_label(unit));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));

    /* These blocks will be dropped, which should cause reg1's live range
     * to be shortened and reg2 to be killed.  We currently rely on the
     * debug output from the optimizer to verify this. */
    int reg2, label2;
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(label2 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg1, reg2, 0));

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label1));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead block 2 (3-4)\n"
        "[info] Killing instruction 4\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] Killing instruction 3\n"
        "[info] Dropping dead block 1 (2-2)\n"
        "[info] Killing instruction 2\n"
        "[info] Dropping branch at 1 to next insn\n"
        "[info] Killing instruction 1\n"
        "[info] Dropping unused label L1\n"
    #endif
    "    0: LOAD_IMM   r1, 0x1\n"
    "    1: NOP\n"
    "    2: NOP\n"
    "    3: NOP\n"
    "    4: NOP\n"
    "    5: NOP\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 3\n"
    "Block 3: 0 --> [5,5] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
