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
    0xFC,0x60,0x18,0x18,  // frsp f3,f3
    0x10,0x22,0x1C,0x20,  // ps_merge00 f1,f2,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: FCVT       r4, r3\n"
    "    4: GET_ALIAS  r5, a3\n"
    "    5: FCMP       r6, r5, r5, UN\n"
    "    6: FCVT       r7, r5\n"
    "    7: SET_ALIAS  a5, r7\n"
    "    8: GOTO_IF_Z  r6, L1\n"
    "    9: BITCAST    r8, r5\n"
    "   10: BFEXT      r9, r8, 51, 1\n"
    "   11: GOTO_IF_NZ r9, L1\n"
    "   12: BITCAST    r10, r7\n"
    "   13: XORI       r11, r10, 4194304\n"
    "   14: BITCAST    r12, r11\n"
    "   15: SET_ALIAS  a5, r12\n"
    "   16: LABEL      L1\n"
    "   17: GET_ALIAS  r13, a5\n"
    "   18: VBUILD2    r14, r13, r4\n"
    "   19: VFCMP      r15, r14, r14, UN\n"
    "   20: VFCVT      r16, r14\n"
    "   21: SET_ALIAS  a6, r16\n"
    "   22: GOTO_IF_Z  r15, L2\n"
    "   23: VEXTRACT   r17, r14, 0\n"
    "   24: VEXTRACT   r18, r14, 1\n"
    "   25: SLLI       r19, r15, 32\n"
    "   26: BITCAST    r20, r17\n"
    "   27: BITCAST    r21, r18\n"
    "   28: NOT        r22, r20\n"
    "   29: NOT        r23, r21\n"
    "   30: ANDI       r24, r22, 4194304\n"
    "   31: ANDI       r25, r23, 4194304\n"
    "   32: VEXTRACT   r26, r16, 0\n"
    "   33: VEXTRACT   r27, r16, 1\n"
    "   34: ZCAST      r28, r24\n"
    "   35: ZCAST      r29, r25\n"
    "   36: SLLI       r30, r28, 29\n"
    "   37: SLLI       r31, r29, 29\n"
    "   38: BITCAST    r32, r26\n"
    "   39: BITCAST    r33, r27\n"
    "   40: AND        r34, r30, r19\n"
    "   41: AND        r35, r31, r15\n"
    "   42: XOR        r36, r32, r34\n"
    "   43: XOR        r37, r33, r35\n"
    "   44: BITCAST    r38, r36\n"
    "   45: BITCAST    r39, r37\n"
    "   46: VBUILD2    r40, r38, r39\n"
    "   47: SET_ALIAS  a6, r40\n"
    "   48: LABEL      L2\n"
    "   49: GET_ALIAS  r41, a6\n"
    "   50: SET_ALIAS  a2, r41\n"
    "   51: FCVT       r42, r4\n"
    "   52: STORE      440(r1), r42\n"
    "   53: SET_ALIAS  a4, r42\n"
    "   54: LOAD_IMM   r43, 8\n"
    "   55: SET_ALIAS  a1, r43\n"
    "   56: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: float32, no bound storage\n"
    "Alias 6: float64[2], no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1,3\n"
    "Block 1: 0 --> [9,11] --> 2,3\n"
    "Block 2: 1 --> [12,15] --> 3\n"
    "Block 3: 2,0,1 --> [16,22] --> 4,5\n"
    "Block 4: 3 --> [23,47] --> 5\n"
    "Block 5: 4,3 --> [48,56] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
