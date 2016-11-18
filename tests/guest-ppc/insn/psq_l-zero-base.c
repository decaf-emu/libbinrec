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
    0xE0,0x20,0xA0,0x10,  // psq_l f1,16(0),1,2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD       r3, 904(r1)\n"
    "    3: BFEXT      r4, r3, 16, 3\n"
    "    4: ANDI       r5, r4, 4\n"
    "    5: GOTO_IF_NZ r5, L2\n"
    "    6: LOAD_BR    r6, 16(r2)\n"
    "    7: LOAD_IMM   r7, 1.0f\n"
    "    8: VBUILD2    r8, r6, r7\n"
    "    9: VFCAST     r9, r8\n"
    "   10: SET_ALIAS  a2, r9\n"
    "   11: GOTO       L1\n"
    "   12: LABEL      L2\n"
    "   13: SLLI       r10, r3, 2\n"
    "   14: SRAI       r11, r10, 26\n"
    "   15: SLLI       r12, r11, 23\n"
    "   16: LOAD_IMM   r13, 0x3F800000\n"
    "   17: SUB        r14, r13, r12\n"
    "   18: BITCAST    r15, r14\n"
    "   19: ANDI       r16, r4, 1\n"
    "   20: GOTO_IF_NZ r16, L3\n"
    "   21: ANDI       r17, r4, 2\n"
    "   22: GOTO_IF_NZ r17, L4\n"
    "   23: LOAD_U8    r18, 16(r2)\n"
    "   24: FSCAST     r19, r18\n"
    "   25: FMUL       r20, r19, r15\n"
    "   26: LOAD_IMM   r21, 1.0f\n"
    "   27: VBUILD2    r22, r20, r21\n"
    "   28: VFCVT      r23, r22\n"
    "   29: SET_ALIAS  a2, r23\n"
    "   30: GOTO       L1\n"
    "   31: LABEL      L4\n"
    "   32: LOAD_S8    r24, 16(r2)\n"
    "   33: FSCAST     r25, r24\n"
    "   34: FMUL       r26, r25, r15\n"
    "   35: LOAD_IMM   r27, 1.0f\n"
    "   36: VBUILD2    r28, r26, r27\n"
    "   37: VFCVT      r29, r28\n"
    "   38: SET_ALIAS  a2, r29\n"
    "   39: GOTO       L1\n"
    "   40: LABEL      L3\n"
    "   41: ANDI       r30, r4, 2\n"
    "   42: GOTO_IF_NZ r30, L5\n"
    "   43: LOAD_U16_BR r31, 16(r2)\n"
    "   44: FSCAST     r32, r31\n"
    "   45: FMUL       r33, r32, r15\n"
    "   46: LOAD_IMM   r34, 1.0f\n"
    "   47: VBUILD2    r35, r33, r34\n"
    "   48: VFCVT      r36, r35\n"
    "   49: SET_ALIAS  a2, r36\n"
    "   50: GOTO       L1\n"
    "   51: LABEL      L5\n"
    "   52: LOAD_S16_BR r37, 16(r2)\n"
    "   53: FSCAST     r38, r37\n"
    "   54: FMUL       r39, r38, r15\n"
    "   55: LOAD_IMM   r40, 1.0f\n"
    "   56: VBUILD2    r41, r39, r40\n"
    "   57: VFCVT      r42, r41\n"
    "   58: SET_ALIAS  a2, r42\n"
    "   59: LABEL      L1\n"
    "   60: LOAD_IMM   r43, 4\n"
    "   61: SET_ALIAS  a1, r43\n"
    "   62: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,5] --> 1,2\n"
    "Block 1: 0 --> [6,11] --> 9\n"
    "Block 2: 0 --> [12,20] --> 3,6\n"
    "Block 3: 2 --> [21,22] --> 4,5\n"
    "Block 4: 3 --> [23,30] --> 9\n"
    "Block 5: 3 --> [31,39] --> 9\n"
    "Block 6: 2 --> [40,42] --> 7,8\n"
    "Block 7: 6 --> [43,50] --> 9\n"
    "Block 8: 6 --> [51,58] --> 9\n"
    "Block 9: 8,1,4,5,7 --> [59,62] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
