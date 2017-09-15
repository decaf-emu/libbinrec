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
    0xF0,0x23,0xAF,0xF0,  // psq_st f1,-16(r3),1,2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a2\n"
    "    4: ZCAST      r5, r4\n"
    "    5: ADD        r6, r2, r5\n"
    "    6: LOAD       r7, 904(r1)\n"
    "    7: BFEXT      r8, r7, 0, 3\n"
    "    8: ANDI       r9, r8, 4\n"
    "    9: GOTO_IF_NZ r9, L2\n"
    "   10: VEXTRACT   r10, r3, 0\n"
    "   11: BITCAST    r11, r10\n"
    "   12: SRLI       r12, r11, 32\n"
    "   13: ZCAST      r13, r12\n"
    "   14: SLLI       r14, r11, 1\n"
    "   15: GOTO_IF_Z  r14, L3\n"
    "   16: LOAD_IMM   r15, 0x701FFFFFFFFFFFFF\n"
    "   17: SGTU       r16, r14, r15\n"
    "   18: GOTO_IF_Z  r16, L4\n"
    "   19: LABEL      L3\n"
    "   20: ANDI       r17, r13, -1073741824\n"
    "   21: BFEXT      r18, r11, 29, 30\n"
    "   22: ZCAST      r19, r18\n"
    "   23: OR         r20, r17, r19\n"
    "   24: STORE_BR   -16(r6), r20\n"
    "   25: GOTO       L5\n"
    "   26: LABEL      L4\n"
    "   27: ANDI       r21, r17, -2147483648\n"
    "   28: STORE_BR   -16(r6), r21\n"
    "   29: LABEL      L5\n"
    "   30: GOTO       L1\n"
    "   31: LABEL      L2\n"
    "   32: SLLI       r22, r7, 18\n"
    "   33: SRAI       r23, r22, 26\n"
    "   34: SLLI       r24, r23, 23\n"
    "   35: ADDI       r25, r24, 1065353216\n"
    "   36: BITCAST    r26, r25\n"
    "   37: ANDI       r27, r8, 2\n"
    "   38: SLLI       r28, r27, 14\n"
    "   39: ANDI       r29, r8, 1\n"
    "   40: XORI       r30, r29, 1\n"
    "   41: SLLI       r31, r30, 3\n"
    "   42: LOAD_IMM   r32, 0\n"
    "   43: LOAD_IMM   r33, 65535\n"
    "   44: SUB        r34, r32, r28\n"
    "   45: SUB        r35, r33, r28\n"
    "   46: SRA        r36, r34, r31\n"
    "   47: SRA        r37, r35, r31\n"
    "   48: VEXTRACT   r38, r3, 0\n"
    "   49: FCVT       r39, r38\n"
    "   50: FMUL       r40, r39, r26\n"
    "   51: BITCAST    r41, r40\n"
    "   52: FTRUNCI    r42, r40\n"
    "   53: SLLI       r43, r41, 1\n"
    "   54: SRLI       r44, r41, 31\n"
    "   55: SELECT     r45, r36, r37, r44\n"
    "   56: SGTUI      r46, r43, -1895825409\n"
    "   57: SELECT     r47, r45, r42, r46\n"
    "   58: SGTS       r48, r47, r37\n"
    "   59: SELECT     r49, r37, r47, r48\n"
    "   60: SLTS       r50, r47, r36\n"
    "   61: SELECT     r51, r36, r49, r50\n"
    "   62: ANDI       r52, r8, 1\n"
    "   63: GOTO_IF_NZ r52, L6\n"
    "   64: STORE_I8   -16(r6), r51\n"
    "   65: GOTO       L1\n"
    "   66: LABEL      L6\n"
    "   67: STORE_I16_BR -16(r6), r51\n"
    "   68: LABEL      L1\n"
    "   69: LOAD_IMM   r53, 4\n"
    "   70: SET_ALIAS  a1, r53\n"
    "   71: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,9] --> 1,6\n"
    "Block 1: 0 --> [10,15] --> 2,3\n"
    "Block 2: 1 --> [16,18] --> 3,4\n"
    "Block 3: 2,1 --> [19,25] --> 5\n"
    "Block 4: 2 --> [26,28] --> 5\n"
    "Block 5: 4,3 --> [29,30] --> 9\n"
    "Block 6: 0 --> [31,63] --> 7,8\n"
    "Block 7: 6 --> [64,65] --> 9\n"
    "Block 8: 6 --> [66,67] --> 9\n"
    "Block 9: 8,5,7 --> [68,71] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
