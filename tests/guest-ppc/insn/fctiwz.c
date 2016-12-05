/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

static const uint8_t input[] = {
    0xFC,0x20,0x10,0x1E,  // fctiwz f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_FAST_FCTIW;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FTRUNCI    r4, r3\n"
    "    4: LOAD_IMM   r5, 2147483647.0\n"
    "    5: FCMP       r6, r3, r5, GE\n"
    "    6: LOAD_IMM   r7, 0x7FFFFFFF\n"
    "    7: SELECT     r8, r7, r4, r6\n"
    "    8: ZCAST      r9, r8\n"
    "    9: BITCAST    r10, r9\n"
    "   10: SET_ALIAS  a2, r10\n"
    "   11: LOAD_IMM   r11, 4\n"
    "   12: SET_ALIAS  a1, r11\n"
    "   13: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "\n"
    "Block 0: <none> --> [0,13] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
