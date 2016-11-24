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
    0x10,0x20,0x10,0x90,  // ps_mr f1,f2
    0xF0,0x23,0xAF,0xF0,  // psq_st f1,-16(r3),1,2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: VFCAST     r4, r3\n"
    "    4: GET_ALIAS  r5, a2\n"
    "    5: ZCAST      r6, r5\n"
    "    6: ADD        r7, r2, r6\n"
    "    7: LOAD       r8, 904(r1)\n"
    "    8: BFEXT      r9, r8, 0, 3\n"
    "    9: ANDI       r10, r9, 4\n"
    "   10: GOTO_IF_NZ r10, L2\n"
    "   11: VEXTRACT   r11, r4, 0\n"
    "   12: BITCAST    r12, r11\n"
    "   13: BFEXT      r13, r12, 23, 8\n"
    "   14: ANDI       r14, r12, -2147483648\n"
    "   15: BITCAST    r15, r14\n"
    "   16: SELECT     r16, r11, r15, r13\n"
    "   17: STORE_BR   -16(r7), r16\n"
    "   18: GOTO       L1\n"
    "   19: LABEL      L2\n"
    "   20: SLLI       r17, r8, 18\n"
    "   21: SRAI       r18, r17, 26\n"
    "   22: SLLI       r19, r18, 23\n"
    "   23: ADDI       r20, r19, 1065353216\n"
    "   24: BITCAST    r21, r20\n"
    "   25: ANDI       r22, r9, 2\n"
    "   26: SLLI       r23, r22, 14\n"
    "   27: ANDI       r24, r9, 1\n"
    "   28: XORI       r25, r24, 1\n"
    "   29: SLLI       r26, r25, 3\n"
    "   30: LOAD_IMM   r27, 0\n"
    "   31: LOAD_IMM   r28, 65535\n"
    "   32: SUB        r29, r27, r23\n"
    "   33: SUB        r30, r28, r23\n"
    "   34: SRA        r31, r29, r26\n"
    "   35: SRA        r32, r30, r26\n"
    "   36: FGETSTATE  r33\n"
    "   37: VEXTRACT   r34, r4, 0\n"
    "   38: FMUL       r35, r34, r21\n"
    "   39: BITCAST    r36, r35\n"
    "   40: FTRUNCI    r37, r35\n"
    "   41: SLLI       r38, r36, 1\n"
    "   42: SRLI       r39, r36, 31\n"
    "   43: SELECT     r40, r31, r32, r39\n"
    "   44: SGTUI      r41, r38, -1895825409\n"
    "   45: SELECT     r42, r40, r37, r41\n"
    "   46: SGTS       r43, r42, r32\n"
    "   47: SELECT     r44, r32, r42, r43\n"
    "   48: SLTS       r45, r42, r31\n"
    "   49: SELECT     r46, r31, r44, r45\n"
    "   50: FSETSTATE  r33\n"
    "   51: ANDI       r47, r9, 1\n"
    "   52: GOTO_IF_NZ r47, L3\n"
    "   53: STORE_I8   -16(r7), r46\n"
    "   54: GOTO       L1\n"
    "   55: LABEL      L3\n"
    "   56: STORE_I16_BR -16(r7), r46\n"
    "   57: LABEL      L1\n"
    "   58: VFCAST     r48, r4\n"
    "   59: SET_ALIAS  a3, r48\n"
    "   60: LOAD_IMM   r49, 8\n"
    "   61: SET_ALIAS  a1, r49\n"
    "   62: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "Alias 4: float64[2] @ 416(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> 1,2\n"
    "Block 1: 0 --> [11,18] --> 5\n"
    "Block 2: 0 --> [19,52] --> 3,4\n"
    "Block 3: 2 --> [53,54] --> 5\n"
    "Block 4: 2 --> [55,56] --> 5\n"
    "Block 5: 4,1,3 --> [57,62] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
