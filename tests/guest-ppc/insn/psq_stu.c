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
    0xF4,0x23,0xAF,0xF0,  // psq_stu f1,-16(r3),1,2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a2\n"
    "    4: ADDI       r5, r4, -16\n"
    "    5: ZCAST      r6, r5\n"
    "    6: ADD        r7, r2, r6\n"
    "    7: LOAD       r8, 904(r1)\n"
    "    8: BFEXT      r9, r8, 0, 3\n"
    "    9: ANDI       r10, r9, 4\n"
    "   10: GOTO_IF_NZ r10, L2\n"
    "   11: VEXTRACT   r11, r3, 0\n"
    "   12: FCAST      r12, r11\n"
    "   13: BITCAST    r13, r12\n"
    "   14: BFEXT      r14, r13, 23, 8\n"
    "   15: ANDI       r15, r13, -2147483648\n"
    "   16: BITCAST    r16, r15\n"
    "   17: SELECT     r17, r12, r16, r14\n"
    "   18: STORE_BR   0(r7), r17\n"
    "   19: GOTO       L1\n"
    "   20: LABEL      L2\n"
    "   21: SLLI       r18, r8, 18\n"
    "   22: SRAI       r19, r18, 26\n"
    "   23: SLLI       r20, r19, 23\n"
    "   24: ADDI       r21, r20, 1065353216\n"
    "   25: BITCAST    r22, r21\n"
    "   26: ANDI       r23, r9, 2\n"
    "   27: SLLI       r24, r23, 14\n"
    "   28: ANDI       r25, r9, 1\n"
    "   29: XORI       r26, r25, 1\n"
    "   30: SLLI       r27, r26, 3\n"
    "   31: LOAD_IMM   r28, 0\n"
    "   32: LOAD_IMM   r29, 65535\n"
    "   33: SUB        r30, r28, r24\n"
    "   34: SUB        r31, r29, r24\n"
    "   35: SRA        r32, r30, r27\n"
    "   36: SRA        r33, r31, r27\n"
    "   37: FGETSTATE  r34\n"
    "   38: VEXTRACT   r35, r3, 0\n"
    "   39: FCVT       r36, r35\n"
    "   40: FMUL       r37, r36, r22\n"
    "   41: BITCAST    r38, r37\n"
    "   42: FROUNDI    r39, r37\n"
    "   43: SLLI       r40, r38, 1\n"
    "   44: SRLI       r41, r38, 31\n"
    "   45: SELECT     r42, r32, r33, r41\n"
    "   46: SGTUI      r43, r40, -1895825409\n"
    "   47: SELECT     r44, r42, r39, r43\n"
    "   48: SGTS       r45, r44, r33\n"
    "   49: SELECT     r46, r33, r44, r45\n"
    "   50: SLTS       r47, r44, r32\n"
    "   51: SELECT     r48, r32, r46, r47\n"
    "   52: FSETSTATE  r34\n"
    "   53: ANDI       r49, r9, 1\n"
    "   54: GOTO_IF_NZ r49, L3\n"
    "   55: STORE_I8   0(r7), r48\n"
    "   56: GOTO       L1\n"
    "   57: LABEL      L3\n"
    "   58: STORE_I16_BR 0(r7), r48\n"
    "   59: LABEL      L1\n"
    "   60: SET_ALIAS  a2, r5\n"
    "   61: LOAD_IMM   r50, 4\n"
    "   62: SET_ALIAS  a1, r50\n"
    "   63: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> 1,2\n"
    "Block 1: 0 --> [11,19] --> 5\n"
    "Block 2: 0 --> [20,54] --> 3,4\n"
    "Block 3: 2 --> [55,56] --> 5\n"
    "Block 4: 2 --> [57,58] --> 5\n"
    "Block 5: 4,1,3 --> [59,63] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
