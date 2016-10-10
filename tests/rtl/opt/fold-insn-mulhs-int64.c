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
                        reg1, 0, 0, UINT64_C(0x0123456789ABCDEF)));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg2, 0, 0, UINT64_C(0xFEDCBA9876543210)));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_MULHS, reg3, reg1, reg2, 0));

    /* Check other combinations of signs. */
    int reg4, reg5, reg6;
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg4, 0, 0, UINT64_C(0xFEDCBA9876543210)));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg5, 0, 0, UINT64_C(0x0123456789ABCDEF)));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_MULHS, reg6, reg4, reg5, 0));

    int reg7, reg8, reg9;
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg7, 0, 0, UINT64_C(0xFEDCBA9876543211)));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg8, 0, 0, UINT64_C(0xFEDCBA9876543210)));
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_MULHS, reg9, reg7, reg8, 0));

    /* Check handling of a negative result with a zero low half. */

    int reg10, reg11, reg12;
    EXPECT(reg10 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg10, 0, 0, UINT64_C(0x123456780000000)));
    EXPECT(reg11 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg11, 0, 0, UINT64_C(0xFEDCBA9800000000)));
    EXPECT(reg12 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_MULHS, reg12, reg10, reg11, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Folded r3 to constant value 0xFFFEB49923CC0953 at 2\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] Folded r6 to constant value 0xFFFEB49923CC0953 at 5\n"
        "[info] r4 no longer used, setting death = birth\n"
        "[info] r5 no longer used, setting death = birth\n"
        "[info] Folded r9 to constant value 0x14B66DC33F6AC at 8\n"
        "[info] r7 no longer used, setting death = birth\n"
        "[info] r8 no longer used, setting death = birth\n"
        "[info] Folded r12 to constant value 0xFFFEB49923506874 at 11\n"
        "[info] r10 no longer used, setting death = birth\n"
        "[info] r11 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_IMM   r1, 0x123456789ABCDEF\n"
    "    1: LOAD_IMM   r2, 0xFEDCBA9876543210\n"
    "    2: LOAD_IMM   r3, 0xFFFEB49923CC0953\n"
    "    3: LOAD_IMM   r4, 0xFEDCBA9876543210\n"
    "    4: LOAD_IMM   r5, 0x123456789ABCDEF\n"
    "    5: LOAD_IMM   r6, 0xFFFEB49923CC0953\n"
    "    6: LOAD_IMM   r7, 0xFEDCBA9876543211\n"
    "    7: LOAD_IMM   r8, 0xFEDCBA9876543210\n"
    "    8: LOAD_IMM   r9, 0x14B66DC33F6AC\n"
    "    9: LOAD_IMM   r10, 0x123456780000000\n"
    "   10: LOAD_IMM   r11, 0xFEDCBA9800000000\n"
    "   11: LOAD_IMM   r12, 0xFFFEB49923506874\n"
    "\n"
    "Block 0: <none> --> [0,11] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
