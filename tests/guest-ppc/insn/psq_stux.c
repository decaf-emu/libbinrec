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
    0x10,0x23,0x25,0x4E,  // psq_stux f1,r3,r4,1,2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: GET_ALIAS  r4, a2\n"
    "    4: GET_ALIAS  r5, a3\n"
    "    5: ADD        r6, r4, r5\n"
    "    6: ZCAST      r7, r6\n"
    "    7: ADD        r8, r2, r7\n"
    "    8: LOAD       r9, 904(r1)\n"
    "    9: BFEXT      r10, r9, 0, 3\n"
    "   10: ANDI       r11, r10, 4\n"
    "   11: GOTO_IF_NZ r11, L2\n"
    "   12: VEXTRACT   r12, r3, 0\n"
    "   13: FCAST      r13, r12\n"
    "   14: BITCAST    r14, r13\n"
    "   15: BFEXT      r15, r14, 23, 8\n"
    "   16: ANDI       r16, r14, -2147483648\n"
    "   17: BITCAST    r17, r16\n"
    "   18: SELECT     r18, r13, r17, r15\n"
    "   19: STORE_BR   0(r8), r18\n"
    "   20: GOTO       L1\n"
    "   21: LABEL      L2\n"
    "   22: SLLI       r19, r9, 18\n"
    "   23: SRAI       r20, r19, 26\n"
    "   24: SLLI       r21, r20, 23\n"
    "   25: ADDI       r22, r21, 1065353216\n"
    "   26: BITCAST    r23, r22\n"
    "   27: ANDI       r24, r10, 2\n"
    "   28: SLLI       r25, r24, 14\n"
    "   29: ANDI       r26, r10, 1\n"
    "   30: XORI       r27, r26, 1\n"
    "   31: SLLI       r28, r27, 3\n"
    "   32: LOAD_IMM   r29, 0\n"
    "   33: LOAD_IMM   r30, 65535\n"
    "   34: SUB        r31, r29, r25\n"
    "   35: SUB        r32, r30, r25\n"
    "   36: SRA        r33, r31, r28\n"
    "   37: SRA        r34, r32, r28\n"
    "   38: FGETSTATE  r35\n"
    "   39: VEXTRACT   r36, r3, 0\n"
    "   40: FCVT       r37, r36\n"
    "   41: FMUL       r38, r37, r23\n"
    "   42: BITCAST    r39, r38\n"
    "   43: FROUNDI    r40, r38\n"
    "   44: SLLI       r41, r39, 1\n"
    "   45: SRLI       r42, r39, 31\n"
    "   46: SELECT     r43, r33, r34, r42\n"
    "   47: SGTUI      r44, r41, -1895825409\n"
    "   48: SELECT     r45, r43, r40, r44\n"
    "   49: SGTS       r46, r45, r34\n"
    "   50: SELECT     r47, r34, r45, r46\n"
    "   51: SLTS       r48, r45, r33\n"
    "   52: SELECT     r49, r33, r47, r48\n"
    "   53: FSETSTATE  r35\n"
    "   54: ANDI       r50, r10, 1\n"
    "   55: GOTO_IF_NZ r50, L3\n"
    "   56: STORE_I8   0(r8), r49\n"
    "   57: GOTO       L1\n"
    "   58: LABEL      L3\n"
    "   59: STORE_I16_BR 0(r8), r49\n"
    "   60: LABEL      L1\n"
    "   61: SET_ALIAS  a2, r6\n"
    "   62: LOAD_IMM   r51, 4\n"
    "   63: SET_ALIAS  a1, r51\n"
    "   64: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,11] --> 1,2\n"
    "Block 1: 0 --> [12,20] --> 5\n"
    "Block 2: 0 --> [21,55] --> 3,4\n"
    "Block 3: 2 --> [56,57] --> 5\n"
    "Block 4: 2 --> [58,59] --> 5\n"
    "Block 5: 4,1,3 --> [60,64] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
