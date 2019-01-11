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
    0xEC,0x21,0x18,0x2A,  // fadds f1,f1,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN
                                    | BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_SINGLE_PREC_INPUTS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FCVT       r4, r3\n"
    "    4: GET_ALIAS  r5, a4\n"
    "    5: FCVT       r6, r5\n"
    "    6: FADD       r7, r4, r6\n"
    "    7: FCVT       r8, r7\n"
    "    8: STORE      408(r1), r8\n"
    "    9: SET_ALIAS  a2, r8\n"
    "   10: LOAD_IMM   r9, 8\n"
    "   11: SET_ALIAS  a1, r9\n"
    "   12: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "\n"
    "Block 0: <none> --> [0,12] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
