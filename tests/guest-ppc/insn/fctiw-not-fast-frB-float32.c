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

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
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
    "    7: FCVT       r8, r4\n"
    "    8: STORE      408(r1), r8\n"
    "    9: SET_ALIAS  a2, r8\n"
    "   10: FGETSTATE  r9\n"
    "   11: FTESTEXC   r10, r9, INVALID\n"
    "   12: GOTO_IF_Z  r10, L1\n"
    "   13: LOAD_IMM   r11, 0.0f\n"
    "   14: FCMP       r12, r4, r11, GT\n"
    "   15: GOTO_IF_Z  r12, L1\n"
    "   16: LOAD_IMM   r13, 1.0609978949885705e-314\n"
    "   17: SET_ALIAS  a2, r13\n"
    "   18: GOTO       L2\n"
    "   19: LABEL      L1\n"
    "   20: SET_ALIAS  a2, r7\n"
    "   21: LABEL      L2\n"
    "   22: GET_ALIAS  r14, a2\n"
    "   23: BITCAST    r15, r4\n"
    "   24: BITCAST    r16, r14\n"
    "   25: LOAD_IMM   r17, 0x80000000\n"
    "   26: SEQ        r18, r15, r17\n"
    "   27: SLLI       r19, r18, 32\n"
    "   28: LOAD_IMM   r20, 0xFFF8000000000000\n"
    "   29: OR         r21, r20, r19\n"
    "   30: OR         r22, r16, r21\n"
    "   31: BITCAST    r23, r22\n"
    "   32: SET_ALIAS  a2, r23\n"
    "   33: FCLEAREXC  r24, r9\n"
    "   34: FSETSTATE  r24\n"
    "   35: LOAD_IMM   r25, 8\n"
    "   36: SET_ALIAS  a1, r25\n"
    "   37: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "\n"
    "Block 0: <none> --> [0,12] --> 1,3\n"
    "Block 1: 0 --> [13,15] --> 2,3\n"
    "Block 2: 1 --> [16,18] --> 4\n"
    "Block 3: 0,1 --> [19,20] --> 4\n"
    "Block 4: 3,2 --> [21,37] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
