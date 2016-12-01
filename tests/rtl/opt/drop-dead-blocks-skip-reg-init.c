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
    int reg1, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));

    /* This block will be dropped, which leads to an invalid code sequence
     * in which reg2 is used without being initialized.  The optimizer
     * should warn about this. */
    int reg2;
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg1, reg2, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead block 1 (2-2)\n"
    #endif
    "[warning] Initialization of r2 at 2 is unreachable\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping branch at 1 to next insn\n"
        "[info] Killing instruction 1\n"
        "[info] Dropping unused label L1\n"
    #endif
    "    0: LOAD_IMM   r1, 0x1\n"
    "    1: NOP\n"
    "    2: LOAD_IMM   r2, 2\n"
    "    3: NOP\n"
    "    4: STORE      0(r1), r2\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 2\n"
    "Block 2: 0 --> [3,4] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
