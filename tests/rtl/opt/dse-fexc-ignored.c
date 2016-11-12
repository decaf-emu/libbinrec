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


static unsigned int opt_flags = BINREC_OPT_DSE | BINREC_OPT_DSE_FP;

static int add_rtl(RTLUnit *unit)
{
    /* With DSE_FP, all of these instructions should be eliminated. */

    int reg1;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));

    int reg2;
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_FCAST, reg2, reg1, 0, 0));

    int reg3, reg4, reg5, reg6;
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_FCVT, reg3, reg1, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FTRUNCI, reg4, reg1, 0, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FADD, reg5, reg1, reg1, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FNMSUB, reg6, reg1, reg1, reg1));

    int reg7, reg8, reg9;
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VBROADCAST, reg7, reg1, 0, 0));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_VFCAST, reg8, reg7, 0, 0));
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_VFCVT, reg9, reg7, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead store to r2 at 1\n"
        "[info] Killing instruction 1\n"
        "[info] Dropping dead store to r3 at 2\n"
        "[info] Killing instruction 2\n"
        "[info] Dropping dead store to r4 at 3\n"
        "[info] Killing instruction 3\n"
        "[info] Dropping dead store to r5 at 4\n"
        "[info] Killing instruction 4\n"
        "[info] Dropping dead store to r6 at 5\n"
        "[info] Killing instruction 5\n"
        "[info] Dropping dead store to r8 at 7\n"
        "[info] Killing instruction 7\n"
        "[info] Dropping dead store to r9 at 8\n"
        "[info] Killing instruction 8\n"
        "[info] r7 no longer used, setting death = birth\n"
        "[info] Killing instruction 6\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] Killing instruction 0\n"
    #endif
    "    0: NOP\n"
    "    1: NOP\n"
    "    2: NOP\n"
    "    3: NOP\n"
    "    4: NOP\n"
    "    5: NOP\n"
    "    6: NOP\n"
    "    7: NOP\n"
    "    8: NOP\n"
    "\n"
    "Block 0: <none> --> [0,8] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
