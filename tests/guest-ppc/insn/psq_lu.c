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
    0xE4,0x23,0xAF,0xF0,  // psq_lu f1,-16(r3),1,2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ADDI       r4, r3, -16\n"
    "    4: ZCAST      r5, r4\n"
    "    5: ADD        r6, r2, r5\n"
    "    6: LOAD       r7, 904(r1)\n"
    "    7: BFEXT      r8, r7, 16, 3\n"
    "    8: ANDI       r9, r8, 4\n"
    "    9: GOTO_IF_NZ r9, L2\n"
    "   10: LOAD_BR    r10, 0(r6)\n"
    "   11: LOAD_IMM   r11, 1.0f\n"
    "   12: VBUILD2    r12, r10, r11\n"
    "   13: VFCVT      r13, r12\n"
    "   14: SET_ALIAS  a3, r13\n"
    "   15: GOTO       L1\n"
    "   16: LABEL      L2\n"
    "   17: SLLI       r14, r7, 2\n"
    "   18: SRAI       r15, r14, 26\n"
    "   19: SLLI       r16, r15, 23\n"
    "   20: LOAD_IMM   r17, 0x3F800000\n"
    "   21: SUB        r18, r17, r16\n"
    "   22: BITCAST    r19, r18\n"
    "   23: ANDI       r20, r8, 1\n"
    "   24: GOTO_IF_NZ r20, L3\n"
    "   25: ANDI       r21, r8, 2\n"
    "   26: GOTO_IF_NZ r21, L4\n"
    "   27: LOAD_U8    r22, 0(r6)\n"
    "   28: FSCAST     r23, r22\n"
    "   29: FMUL       r24, r23, r19\n"
    "   30: LOAD_IMM   r25, 1.0f\n"
    "   31: VBUILD2    r26, r24, r25\n"
    "   32: VFCVT      r27, r26\n"
    "   33: SET_ALIAS  a3, r27\n"
    "   34: GOTO       L1\n"
    "   35: LABEL      L4\n"
    "   36: LOAD_S8    r28, 0(r6)\n"
    "   37: FSCAST     r29, r28\n"
    "   38: FMUL       r30, r29, r19\n"
    "   39: LOAD_IMM   r31, 1.0f\n"
    "   40: VBUILD2    r32, r30, r31\n"
    "   41: VFCVT      r33, r32\n"
    "   42: SET_ALIAS  a3, r33\n"
    "   43: GOTO       L1\n"
    "   44: LABEL      L3\n"
    "   45: ANDI       r34, r8, 2\n"
    "   46: GOTO_IF_NZ r34, L5\n"
    "   47: LOAD_U16_BR r35, 0(r6)\n"
    "   48: FSCAST     r36, r35\n"
    "   49: FMUL       r37, r36, r19\n"
    "   50: LOAD_IMM   r38, 1.0f\n"
    "   51: VBUILD2    r39, r37, r38\n"
    "   52: VFCVT      r40, r39\n"
    "   53: SET_ALIAS  a3, r40\n"
    "   54: GOTO       L1\n"
    "   55: LABEL      L5\n"
    "   56: LOAD_S16_BR r41, 0(r6)\n"
    "   57: FSCAST     r42, r41\n"
    "   58: FMUL       r43, r42, r19\n"
    "   59: LOAD_IMM   r44, 1.0f\n"
    "   60: VBUILD2    r45, r43, r44\n"
    "   61: VFCVT      r46, r45\n"
    "   62: SET_ALIAS  a3, r46\n"
    "   63: LABEL      L1\n"
    "   64: SET_ALIAS  a2, r4\n"
    "   65: LOAD_IMM   r47, 4\n"
    "   66: SET_ALIAS  a1, r47\n"
    "   67: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,9] --> 1,2\n"
    "Block 1: 0 --> [10,15] --> 9\n"
    "Block 2: 0 --> [16,24] --> 3,6\n"
    "Block 3: 2 --> [25,26] --> 4,5\n"
    "Block 4: 3 --> [27,34] --> 9\n"
    "Block 5: 3 --> [35,43] --> 9\n"
    "Block 6: 2 --> [44,46] --> 7,8\n"
    "Block 7: 6 --> [47,54] --> 9\n"
    "Block 8: 6 --> [55,62] --> 9\n"
    "Block 9: 8,1,4,5,7 --> [63,67] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
