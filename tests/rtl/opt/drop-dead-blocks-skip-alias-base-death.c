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
    int reg1, label1, alias;
    EXPECT(label1 = rtl_alloc_label(unit));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias, reg1, 0);
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));

    /* This block will be dropped, which should cause reg1's live range
     * to be shortened even though it's not referenced directly in any
     * instruction.  We currently rely on the debug output from the
     * optimizer to verify this. */
    int reg2;
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg2, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias));

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label1));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead block 1 (2-3)\n"
        "[info] Killing instruction 3\n"
        "[info] r1 death rolled back to 2\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] Killing instruction 2\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] Dropping branch at 1 to next insn\n"
        "[info] Killing instruction 1\n"
        "[info] Dropping unused label L1\n"
    #endif
    "    0: LOAD_IMM   r1, 0x1\n"
    "    1: NOP\n"
    "    2: NOP\n"
    "    3: NOP\n"
    "    4: NOP\n"
    "\n"
    "Alias 1: int32 @ 0(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 2\n"
    "Block 2: 0 --> [4,4] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
