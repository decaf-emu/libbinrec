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
    0x10,0x23,0x21,0x0E,  // psq_stx f1,r3,r4,0,2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: GET_ALIAS  r4, a3\n"
    "    4: ADD        r5, r3, r4\n"
    "    5: ZCAST      r6, r5\n"
    "    6: ADD        r7, r2, r6\n"
    "    7: LOAD       r8, 904(r1)\n"
    "    8: BFEXT      r9, r8, 0, 3\n"
    "    9: ANDI       r10, r9, 4\n"
    "   10: GOTO_IF_NZ r10, L2\n"
    "   11: GET_ALIAS  r11, a4\n"
    "   12: VFCAST     r12, r11\n"
    "   13: VEXTRACT   r13, r12, 0\n"
    "   14: BITCAST    r14, r13\n"
    "   15: BFEXT      r15, r14, 23, 8\n"
    "   16: ANDI       r16, r14, -2147483648\n"
    "   17: BITCAST    r17, r16\n"
    "   18: SELECT     r18, r13, r17, r15\n"
    "   19: STORE_BR   0(r7), r18\n"
    "   20: VEXTRACT   r19, r12, 1\n"
    "   21: BITCAST    r20, r19\n"
    "   22: BFEXT      r21, r20, 23, 8\n"
    "   23: ANDI       r22, r20, -2147483648\n"
    "   24: BITCAST    r23, r22\n"
    "   25: SELECT     r24, r19, r23, r21\n"
    "   26: STORE_BR   4(r7), r24\n"
    "   27: GOTO       L1\n"
    "   28: LABEL      L2\n"
    "   29: SLLI       r25, r8, 18\n"
    "   30: SRAI       r26, r25, 26\n"
    "   31: SLLI       r27, r26, 23\n"
    "   32: ADDI       r28, r27, 1065353216\n"
    "   33: BITCAST    r29, r28\n"
    "   34: ANDI       r30, r9, 2\n"
    "   35: SLLI       r31, r30, 14\n"
    "   36: ANDI       r32, r9, 1\n"
    "   37: XORI       r33, r32, 1\n"
    "   38: SLLI       r34, r33, 3\n"
    "   39: LOAD_IMM   r35, 0\n"
    "   40: LOAD_IMM   r36, 65535\n"
    "   41: SUB        r37, r35, r31\n"
    "   42: SUB        r38, r36, r31\n"
    "   43: SRA        r39, r37, r34\n"
    "   44: SRA        r40, r38, r34\n"
    "   45: FGETSTATE  r41\n"
    "   46: GET_ALIAS  r42, a4\n"
    "   47: VFCVT      r43, r42\n"
    "   48: VEXTRACT   r44, r43, 0\n"
    "   49: FMUL       r45, r44, r29\n"
    "   50: BITCAST    r46, r45\n"
    "   51: FROUNDI    r47, r45\n"
    "   52: SLLI       r48, r46, 1\n"
    "   53: SRLI       r49, r46, 31\n"
    "   54: SELECT     r50, r39, r40, r49\n"
    "   55: SGTUI      r51, r48, -1895825409\n"
    "   56: SELECT     r52, r50, r47, r51\n"
    "   57: SGTS       r53, r52, r40\n"
    "   58: SELECT     r54, r40, r52, r53\n"
    "   59: SLTS       r55, r52, r39\n"
    "   60: SELECT     r56, r39, r54, r55\n"
    "   61: VEXTRACT   r57, r43, 1\n"
    "   62: FMUL       r58, r57, r29\n"
    "   63: BITCAST    r59, r58\n"
    "   64: FROUNDI    r60, r58\n"
    "   65: SLLI       r61, r59, 1\n"
    "   66: SRLI       r62, r59, 31\n"
    "   67: SELECT     r63, r39, r40, r62\n"
    "   68: SGTUI      r64, r61, -1895825409\n"
    "   69: SELECT     r65, r63, r60, r64\n"
    "   70: SGTS       r66, r65, r40\n"
    "   71: SELECT     r67, r40, r65, r66\n"
    "   72: SLTS       r68, r65, r39\n"
    "   73: SELECT     r69, r39, r67, r68\n"
    "   74: FSETSTATE  r41\n"
    "   75: ANDI       r70, r9, 1\n"
    "   76: GOTO_IF_NZ r70, L3\n"
    "   77: STORE_I8   0(r7), r56\n"
    "   78: STORE_I8   1(r7), r69\n"
    "   79: GOTO       L1\n"
    "   80: LABEL      L3\n"
    "   81: STORE_I16_BR 0(r7), r56\n"
    "   82: STORE_I16_BR 2(r7), r69\n"
    "   83: LABEL      L1\n"
    "   84: LOAD_IMM   r71, 4\n"
    "   85: SET_ALIAS  a1, r71\n"
    "   86: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> 1,2\n"
    "Block 1: 0 --> [11,27] --> 5\n"
    "Block 2: 0 --> [28,76] --> 3,4\n"
    "Block 3: 2 --> [77,79] --> 5\n"
    "Block 4: 2 --> [80,82] --> 5\n"
    "Block 5: 4,1,3 --> [83,86] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
