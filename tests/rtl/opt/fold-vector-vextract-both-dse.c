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


static unsigned int opt_flags = BINREC_OPT_DSE
                              | BINREC_OPT_FOLD_CONSTANTS
                              | BINREC_OPT_FOLD_VECTORS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40000000));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VBUILD2, reg3, reg1, reg2, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VEXTRACT, reg4, reg3, 0, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VEXTRACT, reg5, reg3, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg4, reg5, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Reduced VEXTRACT r4 to MOVE from r1\n"
        "[info] Extended r1 live range to 3\n"
        "[info] Folded r4 to constant value 0x3F800000 at 3\n"
        "[info] r1 death rolled back to 2\n"
        "[info] Reduced VEXTRACT r5 to MOVE from r2\n"
        "[info] Extended r2 live range to 4\n"
        "[info] r3 no longer used, setting death = birth\n"
        "[info] Folded r5 to constant value 0x40000000 at 4\n"
        "[info] r2 death rolled back to 2\n"
        "[info] Dropping dead store to r3 at 2\n"
        "[info] Killing instruction 2\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] Killing instruction 1\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] Killing instruction 0\n"
    #endif
    "    0: NOP\n"
    "    1: NOP\n"
    "    2: NOP\n"
    "    3: LOAD_IMM   r4, 1.0f\n"
    "    4: LOAD_IMM   r5, 2.0f\n"
    "    5: NOP        -, r4, r5\n"
    "\n"
    "Block 0: <none> --> [0,5] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
