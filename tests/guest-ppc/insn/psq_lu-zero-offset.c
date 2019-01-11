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
    0xE4,0x23,0xA0,0x00,  // psq_lu f1,0(r3),1,2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN;
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
    "    6: BFEXT      r7, r6, 16, 3\n"
    "    7: ANDI       r8, r7, 4\n"
    "    8: GOTO_IF_NZ r8, L2\n"
    "    9: LOAD_BR    r9, 0(r5)\n"
    "   10: LOAD_IMM   r10, 1.0f\n"
    "   11: VBUILD2    r11, r9, r10\n"
    "   12: VFCVT      r12, r11\n"
    "   13: SET_ALIAS  a3, r12\n"
    "   14: GOTO       L1\n"
    "   15: LABEL      L2\n"
    "   16: SLLI       r13, r6, 2\n"
    "   17: SRAI       r14, r13, 26\n"
    "   18: SLLI       r15, r14, 23\n"
    "   19: LOAD_IMM   r16, 0x3F800000\n"
    "   20: SUB        r17, r16, r15\n"
    "   21: BITCAST    r18, r17\n"
    "   22: ANDI       r19, r7, 1\n"
    "   23: GOTO_IF_NZ r19, L3\n"
    "   24: ANDI       r20, r7, 2\n"
    "   25: GOTO_IF_NZ r20, L4\n"
    "   26: LOAD_U8    r21, 0(r5)\n"
    "   27: FSCAST     r22, r21\n"
    "   28: FMUL       r23, r22, r18\n"
    "   29: LOAD_IMM   r24, 1.0f\n"
    "   30: VBUILD2    r25, r23, r24\n"
    "   31: VFCVT      r26, r25\n"
    "   32: SET_ALIAS  a3, r26\n"
    "   33: GOTO       L1\n"
    "   34: LABEL      L4\n"
    "   35: LOAD_S8    r27, 0(r5)\n"
    "   36: FSCAST     r28, r27\n"
    "   37: FMUL       r29, r28, r18\n"
    "   38: LOAD_IMM   r30, 1.0f\n"
    "   39: VBUILD2    r31, r29, r30\n"
    "   40: VFCVT      r32, r31\n"
    "   41: SET_ALIAS  a3, r32\n"
    "   42: GOTO       L1\n"
    "   43: LABEL      L3\n"
    "   44: ANDI       r33, r7, 2\n"
    "   45: GOTO_IF_NZ r33, L5\n"
    "   46: LOAD_U16_BR r34, 0(r5)\n"
    "   47: FSCAST     r35, r34\n"
    "   48: FMUL       r36, r35, r18\n"
    "   49: LOAD_IMM   r37, 1.0f\n"
    "   50: VBUILD2    r38, r36, r37\n"
    "   51: VFCVT      r39, r38\n"
    "   52: SET_ALIAS  a3, r39\n"
    "   53: GOTO       L1\n"
    "   54: LABEL      L5\n"
    "   55: LOAD_S16_BR r40, 0(r5)\n"
    "   56: FSCAST     r41, r40\n"
    "   57: FMUL       r42, r41, r18\n"
    "   58: LOAD_IMM   r43, 1.0f\n"
    "   59: VBUILD2    r44, r42, r43\n"
    "   60: VFCVT      r45, r44\n"
    "   61: SET_ALIAS  a3, r45\n"
    "   62: LABEL      L1\n"
    "   63: LOAD_IMM   r46, 4\n"
    "   64: SET_ALIAS  a1, r46\n"
    "   65: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1,2\n"
    "Block 1: 0 --> [9,14] --> 9\n"
    "Block 2: 0 --> [15,23] --> 3,6\n"
    "Block 3: 2 --> [24,25] --> 4,5\n"
    "Block 4: 3 --> [26,33] --> 9\n"
    "Block 5: 3 --> [34,42] --> 9\n"
    "Block 6: 2 --> [43,45] --> 7,8\n"
    "Block 7: 6 --> [46,53] --> 9\n"
    "Block 8: 6 --> [54,61] --> 9\n"
    "Block 9: 8,1,4,5,7 --> [62,65] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
