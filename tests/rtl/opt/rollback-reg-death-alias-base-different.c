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


static unsigned int opt_flags = BINREC_OPT_DSE;

static int add_rtl(RTLUnit *unit)
{
    int alias, reg1, reg2, reg3, reg4, reg5, reg6, reg7;
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    /* Check that the alias has the same ID (1) as reg1, so we can verify
     * that the code doesn't treat the alias ID as a register ID when
     * rolling back reg1's death. */
    EXPECT(reg1 == alias);
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 1));
    rtl_set_alias_storage(unit, alias, reg2, 0);
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg3, 0, 0, alias));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 1234));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 5678));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg6, reg4, reg5, reg3));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg7, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg6, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead store to r7 at 6\n"
        "[info] Killing instruction 6\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] Killing instruction 0\n"
    #endif
    "    0: NOP\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a1\n"
    "    3: LOAD_IMM   r4, 1234\n"
    "    4: LOAD_IMM   r5, 5678\n"
    "    5: SELECT     r6, r4, r5, r3\n"
    "    6: NOP\n"
    "    7: NOP        -, r6\n"
    "\n"
    "Alias 1: int32 @ 0(r2)\n"
    "\n"
    "Block 0: <none> --> [0,7] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
