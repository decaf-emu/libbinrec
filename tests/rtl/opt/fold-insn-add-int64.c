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
    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg1, 0, 0, UINT64_C(0x123456789)));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg2, 0, 0, UINT64_C(0x789ABCDEF)));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg3, reg1, reg2, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Folded r3 to constant value 0x8ACF13578 at 2\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] r2 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_IMM   r1, 0x123456789\n"
    "    1: LOAD_IMM   r2, 0x789ABCDEF\n"
    "    2: LOAD_IMM   r3, 0x8ACF13578\n"
    "\n"
    "Block 0: <none> --> [0,2] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
