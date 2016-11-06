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
    int alias1, alias2, reg1, reg2, reg3, reg4, reg5, reg6;
    EXPECT(alias1 = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(alias2 = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    /* Make sure the alias and its base have different IDs so we can
     * verify that the code is looking at the base register and not the
     * alias ID itself. */
    EXPECT(reg1 != alias2);
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    rtl_set_alias_storage(unit, alias2, reg1, 0);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg2, 0, 0, alias2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 1234));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 5678));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg5, reg3, reg4, reg2));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg6, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg5, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead store to r6 at 5\n"
        "[info] Killing instruction 5\n"
        "[info] r1 death rolled back to 1\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: GET_ALIAS  r2, a2\n"
    "    2: LOAD_IMM   r3, 1234\n"
    "    3: LOAD_IMM   r4, 5678\n"
    "    4: SELECT     r5, r3, r4, r2\n"
    "    5: NOP\n"
    "    6: NOP        -, r5\n"
    "\n"
    "Alias 1: int32, no bound storage\n"
    "Alias 2: int32 @ 0(r1)\n"
    "\n"
    "Block 0: <none> --> [0,6] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
