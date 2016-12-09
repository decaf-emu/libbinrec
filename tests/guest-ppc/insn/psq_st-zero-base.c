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
    0xF0,0x20,0xA0,0x10,  // psq_st f1,16(0),1,2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: LOAD       r4, 904(r1)\n"
    "    4: BFEXT      r5, r4, 0, 3\n"
    "    5: ANDI       r6, r5, 4\n"
    "    6: GOTO_IF_NZ r6, L2\n"
    "    7: FGETSTATE  r7\n"
    "    8: VEXTRACT   r8, r3, 0\n"
    "    9: FCAST      r9, r8\n"
    "   10: FSETSTATE  r7\n"
    "   11: BITCAST    r10, r9\n"
    "   12: BFEXT      r11, r10, 23, 8\n"
    "   13: ANDI       r12, r10, -2147483648\n"
    "   14: BITCAST    r13, r12\n"
    "   15: SELECT     r14, r9, r13, r11\n"
    "   16: STORE_BR   16(r2), r14\n"
    "   17: GOTO       L1\n"
    "   18: LABEL      L2\n"
    "   19: SLLI       r15, r4, 18\n"
    "   20: SRAI       r16, r15, 26\n"
    "   21: SLLI       r17, r16, 23\n"
    "   22: ADDI       r18, r17, 1065353216\n"
    "   23: BITCAST    r19, r18\n"
    "   24: ANDI       r20, r5, 2\n"
    "   25: SLLI       r21, r20, 14\n"
    "   26: ANDI       r22, r5, 1\n"
    "   27: XORI       r23, r22, 1\n"
    "   28: SLLI       r24, r23, 3\n"
    "   29: LOAD_IMM   r25, 0\n"
    "   30: LOAD_IMM   r26, 65535\n"
    "   31: SUB        r27, r25, r21\n"
    "   32: SUB        r28, r26, r21\n"
    "   33: SRA        r29, r27, r24\n"
    "   34: SRA        r30, r28, r24\n"
    "   35: FGETSTATE  r31\n"
    "   36: VEXTRACT   r32, r3, 0\n"
    "   37: FCVT       r33, r32\n"
    "   38: FMUL       r34, r33, r19\n"
    "   39: BITCAST    r35, r34\n"
    "   40: FTRUNCI    r36, r34\n"
    "   41: SLLI       r37, r35, 1\n"
    "   42: SRLI       r38, r35, 31\n"
    "   43: SELECT     r39, r29, r30, r38\n"
    "   44: SGTUI      r40, r37, -1895825409\n"
    "   45: SELECT     r41, r39, r36, r40\n"
    "   46: SGTS       r42, r41, r30\n"
    "   47: SELECT     r43, r30, r41, r42\n"
    "   48: SLTS       r44, r41, r29\n"
    "   49: SELECT     r45, r29, r43, r44\n"
    "   50: FSETSTATE  r31\n"
    "   51: ANDI       r46, r5, 1\n"
    "   52: GOTO_IF_NZ r46, L3\n"
    "   53: STORE_I8   16(r2), r45\n"
    "   54: GOTO       L1\n"
    "   55: LABEL      L3\n"
    "   56: STORE_I16_BR 16(r2), r45\n"
    "   57: LABEL      L1\n"
    "   58: LOAD_IMM   r47, 4\n"
    "   59: SET_ALIAS  a1, r47\n"
    "   60: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 1,2\n"
    "Block 1: 0 --> [7,17] --> 5\n"
    "Block 2: 0 --> [18,52] --> 3,4\n"
    "Block 3: 2 --> [53,54] --> 5\n"
    "Block 4: 2 --> [55,56] --> 5\n"
    "Block 5: 4,1,3 --> [57,60] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
