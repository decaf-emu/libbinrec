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
    0xE0,0x23,0x2F,0xF0,  // psq_l f1,-16(r3),0,2
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
    "    9: LOAD_BR    r9, -16(r5)\n"
    "   10: LOAD_BR    r10, -12(r5)\n"
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
    "   26: LOAD_U8    r21, -16(r5)\n"
    "   27: LOAD_U8    r22, -15(r5)\n"
    "   28: FSCAST     r23, r21\n"
    "   29: FMUL       r24, r23, r18\n"
    "   30: FSCAST     r25, r22\n"
    "   31: FMUL       r26, r25, r18\n"
    "   32: VBUILD2    r27, r24, r26\n"
    "   33: VFCVT      r28, r27\n"
    "   34: SET_ALIAS  a3, r28\n"
    "   35: GOTO       L1\n"
    "   36: LABEL      L4\n"
    "   37: LOAD_S8    r29, -16(r5)\n"
    "   38: LOAD_S8    r30, -15(r5)\n"
    "   39: FSCAST     r31, r29\n"
    "   40: FMUL       r32, r31, r18\n"
    "   41: FSCAST     r33, r30\n"
    "   42: FMUL       r34, r33, r18\n"
    "   43: VBUILD2    r35, r32, r34\n"
    "   44: VFCVT      r36, r35\n"
    "   45: SET_ALIAS  a3, r36\n"
    "   46: GOTO       L1\n"
    "   47: LABEL      L3\n"
    "   48: ANDI       r37, r7, 2\n"
    "   49: GOTO_IF_NZ r37, L5\n"
    "   50: LOAD_U16_BR r38, -16(r5)\n"
    "   51: LOAD_U16_BR r39, -14(r5)\n"
    "   52: FSCAST     r40, r38\n"
    "   53: FMUL       r41, r40, r18\n"
    "   54: FSCAST     r42, r39\n"
    "   55: FMUL       r43, r42, r18\n"
    "   56: VBUILD2    r44, r41, r43\n"
    "   57: VFCVT      r45, r44\n"
    "   58: SET_ALIAS  a3, r45\n"
    "   59: GOTO       L1\n"
    "   60: LABEL      L5\n"
    "   61: LOAD_S16_BR r46, -16(r5)\n"
    "   62: LOAD_S16_BR r47, -14(r5)\n"
    "   63: FSCAST     r48, r46\n"
    "   64: FMUL       r49, r48, r18\n"
    "   65: FSCAST     r50, r47\n"
    "   66: FMUL       r51, r50, r18\n"
    "   67: VBUILD2    r52, r49, r51\n"
    "   68: VFCVT      r53, r52\n"
    "   69: SET_ALIAS  a3, r53\n"
    "   70: LABEL      L1\n"
    "   71: LOAD_IMM   r54, 4\n"
    "   72: SET_ALIAS  a1, r54\n"
    "   73: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1,2\n"
    "Block 1: 0 --> [9,14] --> 9\n"
    "Block 2: 0 --> [15,23] --> 3,6\n"
    "Block 3: 2 --> [24,25] --> 4,5\n"
    "Block 4: 3 --> [26,35] --> 9\n"
    "Block 5: 3 --> [36,46] --> 9\n"
    "Block 6: 2 --> [47,49] --> 7,8\n"
    "Block 7: 6 --> [50,59] --> 9\n"
    "Block 8: 6 --> [60,69] --> 9\n"
    "Block 9: 8,1,4,5,7 --> [70,73] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
