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

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD       r6, 904(r1)\n"
    "    6: BFEXT      r7, r6, 0, 3\n"
    "    7: ANDI       r8, r7, 4\n"
    "    8: GOTO_IF_NZ r8, L2\n"
    "    9: GET_ALIAS  r9, a3\n"
    "   10: VEXTRACT   r10, r9, 0\n"
    "   11: FCAST      r11, r10\n"
    "   12: BITCAST    r12, r11\n"
    "   13: BFEXT      r13, r12, 23, 8\n"
    "   14: ANDI       r14, r12, -2147483648\n"
    "   15: BITCAST    r15, r14\n"
    "   16: SELECT     r16, r11, r15, r13\n"
    "   17: STORE_BR   -16(r5), r16\n"
    "   18: GOTO       L1\n"
    "   19: LABEL      L2\n"
    "   20: SLLI       r17, r6, 18\n"
    "   21: SRAI       r18, r17, 26\n"
    "   22: SLLI       r19, r18, 23\n"
    "   23: ADDI       r20, r19, 1065353216\n"
    "   24: BITCAST    r21, r20\n"
    "   25: ANDI       r22, r7, 2\n"
    "   26: SLLI       r23, r22, 14\n"
    "   27: ANDI       r24, r7, 1\n"
    "   28: XORI       r25, r24, 1\n"
    "   29: SLLI       r26, r25, 3\n"
    "   30: LOAD_IMM   r27, 0\n"
    "   31: LOAD_IMM   r28, 65535\n"
    "   32: SUB        r29, r27, r23\n"
    "   33: SUB        r30, r28, r23\n"
    "   34: SRA        r31, r29, r26\n"
    "   35: SRA        r32, r30, r26\n"
    "   36: FGETSTATE  r33\n"
    "   37: GET_ALIAS  r34, a3\n"
    "   38: VEXTRACT   r35, r34, 0\n"
    "   39: FCVT       r36, r35\n"
    "   40: FMUL       r37, r36, r21\n"
    "   41: BITCAST    r38, r37\n"
    "   42: FROUNDI    r39, r37\n"
    "   43: SLLI       r40, r38, 1\n"
    "   44: SRLI       r41, r38, 31\n"
    "   45: SELECT     r42, r31, r32, r41\n"
    "   46: SGTUI      r43, r40, -1895825409\n"
    "   47: SELECT     r44, r42, r39, r43\n"
    "   48: SGTS       r45, r44, r32\n"
    "   49: SELECT     r46, r32, r44, r45\n"
    "   50: SLTS       r47, r44, r31\n"
    "   51: SELECT     r48, r31, r46, r47\n"
    "   52: FSETSTATE  r33\n"
    "   53: ANDI       r49, r7, 1\n"
    "   54: GOTO_IF_NZ r49, L3\n"
    "   55: STORE_I8   -16(r5), r48\n"
    "   56: GOTO       L1\n"
    "   57: LABEL      L3\n"
    "   58: STORE_I16_BR -16(r5), r48\n"
    "   59: LABEL      L1\n"
    "   60: LOAD_IMM   r50, 4\n"
    "   61: SET_ALIAS  a1, r50\n"
    "   62: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1,2\n"
    "Block 1: 0 --> [9,18] --> 5\n"
    "Block 2: 0 --> [19,54] --> 3,4\n"
    "Block 3: 2 --> [55,56] --> 5\n"
    "Block 4: 2 --> [57,58] --> 5\n"
    "Block 5: 4,1,3 --> [59,62] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
