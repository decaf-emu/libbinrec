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
    0xFC,0x20,0x10,0x34,  // frsqrte f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BITCAST    r4, r3\n"
    "    4: SRLI       r5, r4, 32\n"
    "    5: ZCAST      r6, r5\n"
    "    6: LOAD_IMM   r7, 0x8000000000000000\n"
    "    7: AND        r8, r4, r7\n"
    "    8: BFEXT      r9, r6, 0, 20\n"
    "    9: SET_ALIAS  a6, r9\n"
    "   10: BFEXT      r10, r6, 20, 11\n"
    "   11: SET_ALIAS  a5, r10\n"
    "   12: GOTO_IF_Z  r10, L2\n"
    "   13: SEQI       r11, r10, 2047\n"
    "   14: GOTO_IF_NZ r11, L3\n"
    "   15: GOTO_IF_NZ r8, L4\n"
    "   16: GOTO       L5\n"
    "   17: LABEL      L3\n"
    "   18: BFEXT      r12, r4, 0, 52\n"
    "   19: GOTO_IF_NZ r12, L6\n"
    "   20: GOTO_IF_NZ r8, L4\n"
    "   21: LOAD_IMM   r13, 0.0\n"
    "   22: SET_ALIAS  a4, r13\n"
    "   23: GOTO       L1\n"
    "   24: LABEL      L6\n"
    "   25: LOAD_IMM   r14, 0x8000000000000\n"
    "   26: OR         r15, r4, r14\n"
    "   27: BITCAST    r16, r15\n"
    "   28: SET_ALIAS  a4, r16\n"
    "   29: GOTO       L1\n"
    "   30: LABEL      L2\n"
    "   31: BFEXT      r17, r4, 0, 52\n"
    "   32: GOTO_IF_NZ r17, L7\n"
    "   33: LOAD_IMM   r18, 0x7FF0000000000000\n"
    "   34: OR         r19, r8, r18\n"
    "   35: BITCAST    r20, r19\n"
    "   36: SET_ALIAS  a4, r20\n"
    "   37: GOTO       L1\n"
    "   38: LABEL      L7\n"
    "   39: GOTO_IF_NZ r8, L4\n"
    "   40: CLZ        r21, r17\n"
    "   41: ADDI       r22, r21, -11\n"
    "   42: ADDI       r23, r21, -12\n"
    "   43: SLL        r24, r17, r22\n"
    "   44: NEG        r25, r23\n"
    "   45: SET_ALIAS  a5, r25\n"
    "   46: BFEXT      r26, r24, 32, 20\n"
    "   47: ZCAST      r27, r26\n"
    "   48: SET_ALIAS  a6, r27\n"
    "   49: LABEL      L5\n"
    "   50: LOAD       r28, 1016(r1)\n"
    "   51: GET_ALIAS  r29, a6\n"
    "   52: SRLI       r30, r29, 16\n"
    "   53: SLLI       r31, r30, 2\n"
    "   54: GET_ALIAS  r32, a5\n"
    "   55: ANDI       r33, r32, 1\n"
    "   56: XORI       r34, r33, 1\n"
    "   57: SLLI       r35, r34, 6\n"
    "   58: OR         r36, r31, r35\n"
    "   59: LOAD_IMM   r37, 3068\n"
    "   60: SUB        r38, r37, r32\n"
    "   61: SRLI       r39, r38, 1\n"
    "   62: ZCAST      r40, r36\n"
    "   63: ADD        r41, r28, r40\n"
    "   64: LOAD_U16   r42, 2(r41)\n"
    "   65: LOAD_U16   r43, 0(r41)\n"
    "   66: BFEXT      r44, r29, 5, 11\n"
    "   67: MUL        r45, r44, r42\n"
    "   68: SLLI       r46, r43, 11\n"
    "   69: SUB        r47, r46, r45\n"
    "   70: ZCAST      r48, r47\n"
    "   71: ZCAST      r49, r39\n"
    "   72: SLLI       r50, r48, 26\n"
    "   73: SLLI       r51, r49, 52\n"
    "   74: OR         r52, r50, r8\n"
    "   75: OR         r53, r52, r51\n"
    "   76: BITCAST    r54, r53\n"
    "   77: SET_ALIAS  a4, r54\n"
    "   78: GOTO       L1\n"
    "   79: LABEL      L4\n"
    "   80: LOAD_IMM   r55, nan(0x8000000000000)\n"
    "   81: SET_ALIAS  a4, r55\n"
    "   82: GOTO       L1\n"
    "   83: LABEL      L1\n"
    "   84: GET_ALIAS  r56, a4\n"
    "   85: SET_ALIAS  a2, r56\n"
    "   86: LOAD_IMM   r57, 4\n"
    "   87: SET_ALIAS  a1, r57\n"
    "   88: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,12] --> 1,8\n"
    "Block 1: 0 --> [13,14] --> 2,4\n"
    "Block 2: 1 --> [15,15] --> 3,13\n"
    "Block 3: 2 --> [16,16] --> 12\n"
    "Block 4: 1 --> [17,19] --> 5,7\n"
    "Block 5: 4 --> [20,20] --> 6,13\n"
    "Block 6: 5 --> [21,23] --> 14\n"
    "Block 7: 4 --> [24,29] --> 14\n"
    "Block 8: 0 --> [30,32] --> 9,10\n"
    "Block 9: 8 --> [33,37] --> 14\n"
    "Block 10: 8 --> [38,39] --> 11,13\n"
    "Block 11: 10 --> [40,48] --> 12\n"
    "Block 12: 11,3 --> [49,78] --> 14\n"
    "Block 13: 2,5,10 --> [79,82] --> 14\n"
    "Block 14: 6,7,9,12,13 --> [83,88] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
