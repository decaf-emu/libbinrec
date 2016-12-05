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
    0xFC,0x20,0x10,0x18,  // frsp f1,f2
    0xFC,0x20,0x08,0x1C,  // fctiw f1,f1
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_FAST_FCTIW;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FCVT       r4, r3\n"
    "    4: FROUNDI    r5, r4\n"
    "    5: LOAD_IMM   r6, 2.1474836e+09f\n"
    "    6: FCMP       r7, r4, r6, GE\n"
    "    7: LOAD_IMM   r8, 0x7FFFFFFF\n"
    "    8: SELECT     r9, r8, r5, r7\n"
    "    9: ZCAST      r10, r9\n"
    "   10: BITCAST    r11, r10\n"
    "   11: FCVT       r12, r4\n"
    "   12: STORE      408(r1), r12\n"
    "   13: SET_ALIAS  a2, r11\n"
    "   14: LOAD_IMM   r13, 8\n"
    "   15: SET_ALIAS  a1, r13\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "\n"
    "Block 0: <none> --> [0,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
