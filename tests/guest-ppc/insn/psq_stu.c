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
    "   11: FGETSTATE  r11\n"
    "   12: VEXTRACT   r12, r3, 0\n"
    "   13: FCAST      r13, r12\n"
    "   14: FSETSTATE  r11\n"
    "   15: BITCAST    r14, r13\n"
    "   16: BFEXT      r15, r14, 23, 8\n"
    "   17: ANDI       r16, r14, -2147483648\n"
    "   18: BITCAST    r17, r16\n"
    "   19: SELECT     r18, r13, r17, r15\n"
    "   20: STORE_BR   0(r7), r18\n"
    "   21: GOTO       L1\n"
    "   22: LABEL      L2\n"
    "   23: SLLI       r19, r8, 18\n"
    "   24: SRAI       r20, r19, 26\n"
    "   25: SLLI       r21, r20, 23\n"
    "   26: ADDI       r22, r21, 1065353216\n"
    "   27: BITCAST    r23, r22\n"
    "   28: ANDI       r24, r9, 2\n"
    "   29: SLLI       r25, r24, 14\n"
    "   30: ANDI       r26, r9, 1\n"
    "   31: XORI       r27, r26, 1\n"
    "   32: SLLI       r28, r27, 3\n"
    "   33: LOAD_IMM   r29, 0\n"
    "   34: LOAD_IMM   r30, 65535\n"
    "   35: SUB        r31, r29, r25\n"
    "   36: SUB        r32, r30, r25\n"
    "   37: SRA        r33, r31, r28\n"
    "   38: SRA        r34, r32, r28\n"
    "   39: FGETSTATE  r35\n"
    "   40: VEXTRACT   r36, r3, 0\n"
    "   41: FCVT       r37, r36\n"
    "   42: FMUL       r38, r37, r23\n"
    "   43: BITCAST    r39, r38\n"
    "   44: FTRUNCI    r40, r38\n"
    "   45: SLLI       r41, r39, 1\n"
    "   46: SRLI       r42, r39, 31\n"
    "   47: SELECT     r43, r33, r34, r42\n"
    "   48: SGTUI      r44, r41, -1895825409\n"
    "   49: SELECT     r45, r43, r40, r44\n"
    "   50: SGTS       r46, r45, r34\n"
    "   51: SELECT     r47, r34, r45, r46\n"
    "   52: SLTS       r48, r45, r33\n"
    "   53: SELECT     r49, r33, r47, r48\n"
    "   54: FSETSTATE  r35\n"
    "   55: ANDI       r50, r9, 1\n"
    "   56: GOTO_IF_NZ r50, L3\n"
    "   57: STORE_I8   0(r7), r49\n"
    "   58: GOTO       L1\n"
    "   59: LABEL      L3\n"
    "   60: STORE_I16_BR 0(r7), r49\n"
    "   61: LABEL      L1\n"
    "   62: SET_ALIAS  a2, r5\n"
    "   63: LOAD_IMM   r51, 4\n"
    "   64: SET_ALIAS  a1, r51\n"
    "   65: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> 1,2\n"
    "Block 1: 0 --> [11,21] --> 5\n"
    "Block 2: 0 --> [22,56] --> 3,4\n"
    "Block 3: 2 --> [57,58] --> 5\n"
    "Block 4: 2 --> [59,60] --> 5\n"
    "Block 5: 4,1,3 --> [61,65] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
