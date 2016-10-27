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
    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x40000000));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40400000));

    /* Invalid comparison enumerators will be rejected if operand sanity
     * checks are enabled, but check behavior for them anyway. */
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg3, reg1, reg2, RTLFCMP_LT));
    unit->insns[unit->num_insns-1].fcmp = 7;
    unit->regs[reg3].result.fcmp = 7;
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg4, reg2, reg2, RTLFCMP_LT));
    unit->insns[unit->num_insns-1].fcmp = 7;
    unit->regs[reg3].result.fcmp = 7;
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg5, reg2, reg1, RTLFCMP_LT));
    unit->insns[unit->num_insns-1].fcmp = 7;
    unit->regs[reg3].result.fcmp = 7;

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Folded r3 to constant value 0 at 2\n"
        "[info] Folded r4 to constant value 0 at 3\n"
        "[info] Folded r5 to constant value 0 at 4\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] r1 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_IMM   r1, 2.0f\n"
    "    1: LOAD_IMM   r2, 3.0f\n"
    "    2: LOAD_IMM   r3, 0\n"
    "    3: LOAD_IMM   r4, 0\n"
    "    4: LOAD_IMM   r5, 0\n"
    "\n"
    "Block 0: <none> --> [0,4] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
