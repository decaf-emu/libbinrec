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
    "    5: ZCAST      r6, r5\n"
    "    6: BITCAST    r7, r6\n"
    "    7: FGETSTATE  r8\n"
    "    8: FTESTEXC   r9, r8, INVALID\n"
    "    9: GOTO_IF_Z  r9, L1\n"
    "   10: LOAD_IMM   r10, 0.0f\n"
    "   11: FCMP       r11, r4, r10, GT\n"
    "   12: GOTO_IF_Z  r11, L1\n"
    "   13: LOAD_IMM   r12, 1.0609978949885705e-314\n"
    "   14: FCVT       r13, r4\n"
    "   15: STORE      408(r1), r13\n"
    "   16: SET_ALIAS  a2, r12\n"
    "   17: GOTO       L2\n"
    "   18: LABEL      L1\n"
    "   19: SET_ALIAS  a2, r7\n"
    "   20: LABEL      L2\n"
    "   21: FCLEAREXC  r14, r8\n"
    "   22: FSETSTATE  r14\n"
    "   23: LOAD_IMM   r15, 8\n"
    "   24: SET_ALIAS  a1, r15\n"
    "   25: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "\n"
    "Block 0: <none> --> [0,9] --> 1,3\n"
    "Block 1: 0 --> [10,12] --> 2,3\n"
    "Block 2: 1 --> [13,17] --> 4\n"
    "Block 3: 0,1 --> [18,19] --> 4\n"
    "Block 4: 3,2 --> [20,25] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
