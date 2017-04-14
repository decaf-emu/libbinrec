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
    0xEC,0x40,0x10,0x30,  // fres f2,f2
    0x10,0x22,0x18,0x2A,  // ps_add f1,f2,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_NATIVE_RECIPROCAL;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: VEXTRACT   r4, r3, 0\n"
    "    4: FCMP       r5, r4, r4, UN\n"
    "    5: FCVT       r6, r4\n"
    "    6: SET_ALIAS  a5, r6\n"
    "    7: GOTO_IF_Z  r5, L1\n"
    "    8: BITCAST    r7, r4\n"
    "    9: BFEXT      r8, r7, 51, 1\n"
    "   10: GOTO_IF_NZ r8, L1\n"
    "   11: BITCAST    r9, r6\n"
    "   12: XORI       r10, r9, 4194304\n"
    "   13: BITCAST    r11, r10\n"
    "   14: SET_ALIAS  a5, r11\n"
    "   15: LABEL      L1\n"
    "   16: GET_ALIAS  r12, a5\n"
    "   17: LOAD_IMM   r13, 1.0f\n"
    "   18: FDIV       r14, r13, r12\n"
    "   19: FCVT       r15, r14\n"
    "   20: VBROADCAST r16, r15\n"
    "   21: GET_ALIAS  r17, a4\n"
    "   22: FADD       r18, r16, r17\n"
    "   23: VFCVT      r19, r18\n"
    "   24: VFCVT      r20, r19\n"
    "   25: SET_ALIAS  a2, r20\n"
    "   26: FCVT       r21, r14\n"
    "   27: VBROADCAST r22, r21\n"
    "   28: SET_ALIAS  a3, r22\n"
    "   29: LOAD_IMM   r23, 8\n"
    "   30: SET_ALIAS  a1, r23\n"
    "   31: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 1,3\n"
    "Block 1: 0 --> [8,10] --> 2,3\n"
    "Block 2: 1 --> [11,14] --> 3\n"
    "Block 3: 2,0,1 --> [15,31] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
