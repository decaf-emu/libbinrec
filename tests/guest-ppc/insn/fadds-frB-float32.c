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
    0xEC,0x63,0x18,0x2A,  // fadds f3,f3,f3
    0xEC,0x22,0x18,0x2A,  // fadds f1,f2,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: FADD       r4, r3, r3\n"
    "    4: FCVT       r5, r4\n"
    "    5: GET_ALIAS  r6, a3\n"
    "    6: FCVT       r7, r5\n"
    "    7: FADD       r8, r6, r7\n"
    "    8: FCVT       r9, r8\n"
    "    9: FCVT       r10, r9\n"
    "   10: STORE      408(r1), r10\n"
    "   11: SET_ALIAS  a2, r10\n"
    "   12: FCVT       r11, r5\n"
    "   13: STORE      440(r1), r11\n"
    "   14: SET_ALIAS  a4, r11\n"
    "   15: LOAD_IMM   r12, 8\n"
    "   16: SET_ALIAS  a1, r12\n"
    "   17: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "\n"
    "Block 0: <none> --> [0,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
