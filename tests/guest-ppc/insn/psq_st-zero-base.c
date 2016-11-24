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
    "    7: VEXTRACT   r7, r3, 0\n"
    "    8: FCAST      r8, r7\n"
    "    9: BITCAST    r9, r8\n"
    "   10: BFEXT      r10, r9, 23, 8\n"
    "   11: ANDI       r11, r9, -2147483648\n"
    "   12: BITCAST    r12, r11\n"
    "   13: SELECT     r13, r8, r12, r10\n"
    "   14: STORE_BR   16(r2), r13\n"
    "   15: GOTO       L1\n"
    "   16: LABEL      L2\n"
    "   17: SLLI       r14, r4, 18\n"
    "   18: SRAI       r15, r14, 26\n"
    "   19: SLLI       r16, r15, 23\n"
    "   20: ADDI       r17, r16, 1065353216\n"
    "   21: BITCAST    r18, r17\n"
    "   22: ANDI       r19, r5, 2\n"
    "   23: SLLI       r20, r19, 14\n"
    "   24: ANDI       r21, r5, 1\n"
    "   25: XORI       r22, r21, 1\n"
    "   26: SLLI       r23, r22, 3\n"
    "   27: LOAD_IMM   r24, 0\n"
    "   28: LOAD_IMM   r25, 65535\n"
    "   29: SUB        r26, r24, r20\n"
    "   30: SUB        r27, r25, r20\n"
    "   31: SRA        r28, r26, r23\n"
    "   32: SRA        r29, r27, r23\n"
    "   33: FGETSTATE  r30\n"
    "   34: VEXTRACT   r31, r3, 0\n"
    "   35: FCVT       r32, r31\n"
    "   36: FMUL       r33, r32, r18\n"
    "   37: BITCAST    r34, r33\n"
    "   38: FTRUNCI    r35, r33\n"
    "   39: SLLI       r36, r34, 1\n"
    "   40: SRLI       r37, r34, 31\n"
    "   41: SELECT     r38, r28, r29, r37\n"
    "   42: SGTUI      r39, r36, -1895825409\n"
    "   43: SELECT     r40, r38, r35, r39\n"
    "   44: SGTS       r41, r40, r29\n"
    "   45: SELECT     r42, r29, r40, r41\n"
    "   46: SLTS       r43, r40, r28\n"
    "   47: SELECT     r44, r28, r42, r43\n"
    "   48: FSETSTATE  r30\n"
    "   49: ANDI       r45, r5, 1\n"
    "   50: GOTO_IF_NZ r45, L3\n"
    "   51: STORE_I8   16(r2), r44\n"
    "   52: GOTO       L1\n"
    "   53: LABEL      L3\n"
    "   54: STORE_I16_BR 16(r2), r44\n"
    "   55: LABEL      L1\n"
    "   56: LOAD_IMM   r46, 4\n"
    "   57: SET_ALIAS  a1, r46\n"
    "   58: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 1,2\n"
    "Block 1: 0 --> [7,15] --> 5\n"
    "Block 2: 0 --> [16,50] --> 3,4\n"
    "Block 3: 2 --> [51,52] --> 5\n"
    "Block 4: 2 --> [53,54] --> 5\n"
    "Block 5: 4,1,3 --> [55,58] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
