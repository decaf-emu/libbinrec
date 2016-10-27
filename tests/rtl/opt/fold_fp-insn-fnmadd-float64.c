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


static unsigned int opt_flags = BINREC_OPT_FOLD_CONSTANTS
                              | BINREC_OPT_FOLD_FP_CONSTANTS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg1, 0, 0, UINT64_C(0x4000000000000000)));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg2, 0, 0, UINT64_C(0x4008000000000000)));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg3, 0, 0, UINT64_C(0x4014000000000000)));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_FNMADD, reg4, reg1, reg2, reg3));

    /* Also verify that it's a true FMA and not separate operations (see
     * fmadd-float32 test). */
    int reg5, reg6, reg7, reg8;
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg5, 0, 0, UINT64_C(0x3FF0000000000001)));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg6, 0, 0, UINT64_C(0x3FF4000000000000)));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg7, 0, 0, UINT64_C(0x3C98000000000000)));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_FNMADD, reg8, reg5, reg6, reg7));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Folded r4 to constant value 0xC026000000000000 at 3\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] r3 no longer used, setting death = birth\n"
        "[info] Folded r8 to constant value 0xBFF4000000000002 at 7\n"
        "[info] r5 no longer used, setting death = birth\n"
        "[info] r6 no longer used, setting death = birth\n"
        "[info] r7 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_IMM   r1, 2.0\n"
    "    1: LOAD_IMM   r2, 3.0\n"
    "    2: LOAD_IMM   r3, 5.0\n"
    "    3: LOAD_IMM   r4, -11.0\n"
    "    4: LOAD_IMM   r5, 1.0000000000000002\n"
    "    5: LOAD_IMM   r6, 1.25\n"
    "    6: LOAD_IMM   r7, 8.3266726846886741e-17\n"
    "    7: LOAD_IMM   r8, -1.2500000000000004\n"
    "\n"
    "Block 0: <none> --> [0,7] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
