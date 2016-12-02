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


static unsigned int opt_flags = BINREC_OPT_FOLD_VECTORS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, reg5, reg6;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40000000));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VBUILD2, reg3, reg1, reg2, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 0x40800000));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VINSERT, reg5, reg3, reg4, 1));
    /* Touch reg3 to block VINSERT optimization. */
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    /* This will fail to fold because we don't recurse through VINSERT. */
    EXPECT(rtl_add_insn(unit, RTLOP_VEXTRACT, reg6, reg5, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    "    0: LOAD_IMM   r1, 1.0f\n"
    "    1: LOAD_IMM   r2, 2.0f\n"
    "    2: VBUILD2    r3, r1, r2\n"
    "    3: LOAD_IMM   r4, 4.0f\n"
    "    4: VINSERT    r5, r3, r4, 1\n"
    "    5: NOP        -, r3\n"
    "    6: VEXTRACT   r6, r5, 0\n"
    "\n"
    "Block 0: <none> --> [0,6] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
