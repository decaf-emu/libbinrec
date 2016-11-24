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
    0xFC,0x20,0x10,0x1C,  // fctiw f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FROUNDI    r4, r3\n"
    "    4: ZCAST      r5, r4\n"
    "    5: BITCAST    r6, r5\n"
    "    6: FGETSTATE  r7\n"
    "    7: FTESTEXC   r8, r7, INVALID\n"
    "    8: GOTO_IF_Z  r8, L1\n"
    "    9: LOAD_IMM   r9, 0.0\n"
    "   10: FCMP       r10, r3, r9, GT\n"
    "   11: GOTO_IF_Z  r10, L1\n"
    "   12: LOAD_IMM   r11, 1.0609978949885705e-314\n"
    "   13: SET_ALIAS  a2, r11\n"
    "   14: GOTO       L2\n"
    "   15: LABEL      L1\n"
    "   16: SET_ALIAS  a2, r6\n"
    "   17: LABEL      L2\n"
    "   18: GET_ALIAS  r12, a2\n"
    "   19: BITCAST    r13, r3\n"
    "   20: BITCAST    r14, r12\n"
    "   21: LOAD_IMM   r15, 0x8000000000000000\n"
    "   22: SEQ        r16, r13, r15\n"
    "   23: SLLI       r17, r16, 32\n"
    "   24: LOAD_IMM   r18, 0xFFF8000000000000\n"
    "   25: OR         r19, r18, r17\n"
    "   26: OR         r20, r14, r19\n"
    "   27: BITCAST    r21, r20\n"
    "   28: SET_ALIAS  a2, r21\n"
    "   29: FCLEAREXC  r22, r7\n"
    "   30: FSETSTATE  r22\n"
    "   31: LOAD_IMM   r23, 4\n"
    "   32: SET_ALIAS  a1, r23\n"
    "   33: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1,3\n"
    "Block 1: 0 --> [9,11] --> 2,3\n"
    "Block 2: 1 --> [12,14] --> 4\n"
    "Block 3: 0,1 --> [15,16] --> 4\n"
    "Block 4: 3,2 --> [17,33] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
