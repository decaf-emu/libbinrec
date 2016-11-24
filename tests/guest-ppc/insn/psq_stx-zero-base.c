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
    0x10,0x20,0x1D,0x0E,  // psq_stx f1,0,r3,1,2
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
    "    4: ZCAST      r5, r4\n"
    "    5: ADD        r6, r2, r5\n"
    "    6: LOAD       r7, 904(r1)\n"
    "    7: BFEXT      r8, r7, 0, 3\n"
    "    8: ANDI       r9, r8, 4\n"
    "    9: GOTO_IF_NZ r9, L2\n"
    "   10: VEXTRACT   r10, r3, 0\n"
    "   11: FCAST      r11, r10\n"
    "   12: BITCAST    r12, r11\n"
    "   13: BFEXT      r13, r12, 23, 8\n"
    "   14: ANDI       r14, r12, -2147483648\n"
    "   15: BITCAST    r15, r14\n"
    "   16: SELECT     r16, r11, r15, r13\n"
    "   17: STORE_BR   0(r6), r16\n"
    "   18: GOTO       L1\n"
    "   19: LABEL      L2\n"
    "   20: SLLI       r17, r7, 18\n"
    "   21: SRAI       r18, r17, 26\n"
    "   22: SLLI       r19, r18, 23\n"
    "   23: ADDI       r20, r19, 1065353216\n"
    "   24: BITCAST    r21, r20\n"
    "   25: ANDI       r22, r8, 2\n"
    "   26: SLLI       r23, r22, 14\n"
    "   27: ANDI       r24, r8, 1\n"
    "   28: XORI       r25, r24, 1\n"
    "   29: SLLI       r26, r25, 3\n"
    "   30: LOAD_IMM   r27, 0\n"
    "   31: LOAD_IMM   r28, 65535\n"
    "   32: SUB        r29, r27, r23\n"
    "   33: SUB        r30, r28, r23\n"
    "   34: SRA        r31, r29, r26\n"
    "   35: SRA        r32, r30, r26\n"
    "   36: FGETSTATE  r33\n"
    "   37: VEXTRACT   r34, r3, 0\n"
    "   38: FCVT       r35, r34\n"
    "   39: FMUL       r36, r35, r21\n"
    "   40: BITCAST    r37, r36\n"
    "   41: FTRUNCI    r38, r36\n"
    "   42: SLLI       r39, r37, 1\n"
    "   43: SRLI       r40, r37, 31\n"
    "   44: SELECT     r41, r31, r32, r40\n"
    "   45: SGTUI      r42, r39, -1895825409\n"
    "   46: SELECT     r43, r41, r38, r42\n"
    "   47: SGTS       r44, r43, r32\n"
    "   48: SELECT     r45, r32, r43, r44\n"
    "   49: SLTS       r46, r43, r31\n"
    "   50: SELECT     r47, r31, r45, r46\n"
    "   51: FSETSTATE  r33\n"
    "   52: ANDI       r48, r8, 1\n"
    "   53: GOTO_IF_NZ r48, L3\n"
    "   54: STORE_I8   0(r6), r47\n"
    "   55: GOTO       L1\n"
    "   56: LABEL      L3\n"
    "   57: STORE_I16_BR 0(r6), r47\n"
    "   58: LABEL      L1\n"
    "   59: LOAD_IMM   r49, 4\n"
    "   60: SET_ALIAS  a1, r49\n"
    "   61: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,9] --> 1,2\n"
    "Block 1: 0 --> [10,18] --> 5\n"
    "Block 2: 0 --> [19,53] --> 3,4\n"
    "Block 3: 2 --> [54,55] --> 5\n"
    "Block 4: 2 --> [56,57] --> 5\n"
    "Block 5: 4,1,3 --> [58,61] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
