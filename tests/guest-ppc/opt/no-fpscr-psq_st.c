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
    "   11: FCMP       r11, r10, r10, UN\n"
    "   12: FCVT       r12, r10\n"
    "   13: SET_ALIAS  a4, r12\n"
    "   14: GOTO_IF_Z  r11, L3\n"
    "   15: BITCAST    r13, r10\n"
    "   16: BFEXT      r14, r13, 51, 1\n"
    "   17: GOTO_IF_NZ r14, L3\n"
    "   18: BITCAST    r15, r12\n"
    "   19: XORI       r16, r15, 4194304\n"
    "   20: BITCAST    r17, r16\n"
    "   21: SET_ALIAS  a4, r17\n"
    "   22: LABEL      L3\n"
    "   23: GET_ALIAS  r18, a4\n"
    "   24: BITCAST    r19, r18\n"
    "   25: BFEXT      r20, r19, 23, 8\n"
    "   26: ANDI       r21, r19, -2147483648\n"
    "   27: BITCAST    r22, r21\n"
    "   28: SELECT     r23, r18, r22, r20\n"
    "   29: STORE_BR   -16(r6), r23\n"
    "   30: GOTO       L1\n"
    "   31: LABEL      L2\n"
    "   32: SLLI       r24, r7, 18\n"
    "   33: SRAI       r25, r24, 26\n"
    "   34: SLLI       r26, r25, 23\n"
    "   35: ADDI       r27, r26, 1065353216\n"
    "   36: BITCAST    r28, r27\n"
    "   37: ANDI       r29, r8, 2\n"
    "   38: SLLI       r30, r29, 14\n"
    "   39: ANDI       r31, r8, 1\n"
    "   40: XORI       r32, r31, 1\n"
    "   41: SLLI       r33, r32, 3\n"
    "   42: LOAD_IMM   r34, 0\n"
    "   43: LOAD_IMM   r35, 65535\n"
    "   44: SUB        r36, r34, r30\n"
    "   45: SUB        r37, r35, r30\n"
    "   46: SRA        r38, r36, r33\n"
    "   47: SRA        r39, r37, r33\n"
    "   48: VEXTRACT   r40, r3, 0\n"
    "   49: FCVT       r41, r40\n"
    "   50: FMUL       r42, r41, r28\n"
    "   51: BITCAST    r43, r42\n"
    "   52: FTRUNCI    r44, r42\n"
    "   53: SLLI       r45, r43, 1\n"
    "   54: SRLI       r46, r43, 31\n"
    "   55: SELECT     r47, r38, r39, r46\n"
    "   56: SGTUI      r48, r45, -1895825409\n"
    "   57: SELECT     r49, r47, r44, r48\n"
    "   58: SGTS       r50, r49, r39\n"
    "   59: SELECT     r51, r39, r49, r50\n"
    "   60: SLTS       r52, r49, r38\n"
    "   61: SELECT     r53, r38, r51, r52\n"
    "   62: ANDI       r54, r8, 1\n"
    "   63: GOTO_IF_NZ r54, L4\n"
    "   64: STORE_I8   -16(r6), r53\n"
    "   65: GOTO       L1\n"
    "   66: LABEL      L4\n"
    "   67: STORE_I16_BR -16(r6), r53\n"
    "   68: LABEL      L1\n"
    "   69: LOAD_IMM   r55, 4\n"
    "   70: SET_ALIAS  a1, r55\n"
    "   71: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "Alias 4: float32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,9] --> 1,5\n"
    "Block 1: 0 --> [10,14] --> 2,4\n"
    "Block 2: 1 --> [15,17] --> 3,4\n"
    "Block 3: 2 --> [18,21] --> 4\n"
    "Block 4: 3,1,2 --> [22,30] --> 8\n"
    "Block 5: 0 --> [31,63] --> 6,7\n"
    "Block 6: 5 --> [64,65] --> 8\n"
    "Block 7: 5 --> [66,67] --> 8\n"
    "Block 8: 7,4,6 --> [68,71] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
