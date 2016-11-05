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
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 0x80000000));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 0xFFFFFFFF));

    int reg;

    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg2, RTLFCMP_LT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg2, RTLFCMP_LE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg2, RTLFCMP_GT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg2, RTLFCMP_GE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg2, RTLFCMP_EQ));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg2, RTLFCMP_UN));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg2, RTLFCMP_NLT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg2, RTLFCMP_NLE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg2, RTLFCMP_NGT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg2, RTLFCMP_NGE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg2, RTLFCMP_NEQ));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg2, RTLFCMP_NUN));

    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg3, reg4, RTLFCMP_LT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg3, reg4, RTLFCMP_LE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg3, reg4, RTLFCMP_GT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg3, reg4, RTLFCMP_GE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg3, reg4, RTLFCMP_EQ));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg3, reg4, RTLFCMP_UN));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg3, reg4, RTLFCMP_NLT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg3, reg4, RTLFCMP_NLE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg3, reg4, RTLFCMP_NGT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg3, reg4, RTLFCMP_NGE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg3, reg4, RTLFCMP_NEQ));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg3, reg4, RTLFCMP_NUN));

    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg2, reg1, RTLFCMP_LT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg2, reg1, RTLFCMP_LE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg2, reg1, RTLFCMP_GT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg2, reg1, RTLFCMP_GE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg2, reg1, RTLFCMP_EQ));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg2, reg1, RTLFCMP_UN));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg2, reg1, RTLFCMP_NLT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg2, reg1, RTLFCMP_NLE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg2, reg1, RTLFCMP_NGT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg2, reg1, RTLFCMP_NGE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg2, reg1, RTLFCMP_NEQ));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg2, reg1, RTLFCMP_NUN));

    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg5, RTLFCMP_LT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg5, RTLFCMP_LE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg5, RTLFCMP_GT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg5, RTLFCMP_GE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg5, RTLFCMP_EQ));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg5, RTLFCMP_UN));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg5, RTLFCMP_NLT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg5, RTLFCMP_NLE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg5, RTLFCMP_NGT));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg5, RTLFCMP_NGE));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg5, RTLFCMP_NEQ));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg, reg1, reg5, RTLFCMP_NUN));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Folded r6 to constant value 1 at 5\n"
        "[info] Folded r7 to constant value 1 at 6\n"
        "[info] Folded r8 to constant value 0 at 7\n"
        "[info] Folded r9 to constant value 0 at 8\n"
        "[info] Folded r10 to constant value 0 at 9\n"
        "[info] Folded r11 to constant value 0 at 10\n"
        "[info] Folded r12 to constant value 0 at 11\n"
        "[info] Folded r13 to constant value 0 at 12\n"
        "[info] Folded r14 to constant value 1 at 13\n"
        "[info] Folded r15 to constant value 1 at 14\n"
        "[info] Folded r16 to constant value 1 at 15\n"
        "[info] Folded r17 to constant value 1 at 16\n"
        "[info] Folded r18 to constant value 0 at 17\n"
        "[info] Folded r19 to constant value 1 at 18\n"
        "[info] Folded r20 to constant value 0 at 19\n"
        "[info] Folded r21 to constant value 1 at 20\n"
        "[info] Folded r22 to constant value 1 at 21\n"
        "[info] Folded r23 to constant value 0 at 22\n"
        "[info] Folded r24 to constant value 1 at 23\n"
        "[info] Folded r25 to constant value 0 at 24\n"
        "[info] Folded r26 to constant value 1 at 25\n"
        "[info] Folded r27 to constant value 0 at 26\n"
        "[info] Folded r28 to constant value 0 at 27\n"
        "[info] Folded r29 to constant value 1 at 28\n"
        "[info] r3 no longer used, setting death = birth\n"
        "[info] r4 no longer used, setting death = birth\n"
        "[info] Folded r30 to constant value 0 at 29\n"
        "[info] Folded r31 to constant value 0 at 30\n"
        "[info] Folded r32 to constant value 1 at 31\n"
        "[info] Folded r33 to constant value 1 at 32\n"
        "[info] Folded r34 to constant value 0 at 33\n"
        "[info] Folded r35 to constant value 0 at 34\n"
        "[info] Folded r36 to constant value 1 at 35\n"
        "[info] Folded r37 to constant value 1 at 36\n"
        "[info] Folded r38 to constant value 0 at 37\n"
        "[info] Folded r39 to constant value 0 at 38\n"
        "[info] Folded r40 to constant value 1 at 39\n"
        "[info] Folded r41 to constant value 1 at 40\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] Folded r42 to constant value 0 at 41\n"
        "[info] Folded r43 to constant value 0 at 42\n"
        "[info] Folded r44 to constant value 0 at 43\n"
        "[info] Folded r45 to constant value 0 at 44\n"
        "[info] Folded r46 to constant value 0 at 45\n"
        "[info] Folded r47 to constant value 1 at 46\n"
        "[info] Folded r48 to constant value 1 at 47\n"
        "[info] Folded r49 to constant value 1 at 48\n"
        "[info] Folded r50 to constant value 1 at 49\n"
        "[info] Folded r51 to constant value 1 at 50\n"
        "[info] Folded r52 to constant value 1 at 51\n"
        "[info] Folded r53 to constant value 0 at 52\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] r5 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_IMM   r1, 2.0f\n"
    "    1: LOAD_IMM   r2, 3.0f\n"
    "    2: LOAD_IMM   r3, 0.0f\n"
    "    3: LOAD_IMM   r4, -0.0f\n"
    "    4: LOAD_IMM   r5, -nan(0x7FFFFF)\n"
    "    5: LOAD_IMM   r6, 1\n"
    "    6: LOAD_IMM   r7, 1\n"
    "    7: LOAD_IMM   r8, 0\n"
    "    8: LOAD_IMM   r9, 0\n"
    "    9: LOAD_IMM   r10, 0\n"
    "   10: LOAD_IMM   r11, 0\n"
    "   11: LOAD_IMM   r12, 0\n"
    "   12: LOAD_IMM   r13, 0\n"
    "   13: LOAD_IMM   r14, 1\n"
    "   14: LOAD_IMM   r15, 1\n"
    "   15: LOAD_IMM   r16, 1\n"
    "   16: LOAD_IMM   r17, 1\n"
    "   17: LOAD_IMM   r18, 0\n"
    "   18: LOAD_IMM   r19, 1\n"
    "   19: LOAD_IMM   r20, 0\n"
    "   20: LOAD_IMM   r21, 1\n"
    "   21: LOAD_IMM   r22, 1\n"
    "   22: LOAD_IMM   r23, 0\n"
    "   23: LOAD_IMM   r24, 1\n"
    "   24: LOAD_IMM   r25, 0\n"
    "   25: LOAD_IMM   r26, 1\n"
    "   26: LOAD_IMM   r27, 0\n"
    "   27: LOAD_IMM   r28, 0\n"
    "   28: LOAD_IMM   r29, 1\n"
    "   29: LOAD_IMM   r30, 0\n"
    "   30: LOAD_IMM   r31, 0\n"
    "   31: LOAD_IMM   r32, 1\n"
    "   32: LOAD_IMM   r33, 1\n"
    "   33: LOAD_IMM   r34, 0\n"
    "   34: LOAD_IMM   r35, 0\n"
    "   35: LOAD_IMM   r36, 1\n"
    "   36: LOAD_IMM   r37, 1\n"
    "   37: LOAD_IMM   r38, 0\n"
    "   38: LOAD_IMM   r39, 0\n"
    "   39: LOAD_IMM   r40, 1\n"
    "   40: LOAD_IMM   r41, 1\n"
    "   41: LOAD_IMM   r42, 0\n"
    "   42: LOAD_IMM   r43, 0\n"
    "   43: LOAD_IMM   r44, 0\n"
    "   44: LOAD_IMM   r45, 0\n"
    "   45: LOAD_IMM   r46, 0\n"
    "   46: LOAD_IMM   r47, 1\n"
    "   47: LOAD_IMM   r48, 1\n"
    "   48: LOAD_IMM   r49, 1\n"
    "   49: LOAD_IMM   r50, 1\n"
    "   50: LOAD_IMM   r51, 1\n"
    "   51: LOAD_IMM   r52, 1\n"
    "   52: LOAD_IMM   r53, 0\n"
    "\n"
    "Block 0: <none> --> [0,52] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
