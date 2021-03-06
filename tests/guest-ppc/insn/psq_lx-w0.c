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
    0x10,0x23,0x21,0x0C,  // psq_lx f1,r3,r4,0,2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN;
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
    "    8: BFEXT      r9, r8, 16, 3\n"
    "    9: ANDI       r10, r9, 4\n"
    "   10: GOTO_IF_NZ r10, L2\n"
    "   11: LOAD_BR    r11, 0(r7)\n"
    "   12: LOAD_BR    r12, 4(r7)\n"
    "   13: VBUILD2    r13, r11, r12\n"
    "   14: VFCVT      r14, r13\n"
    "   15: SET_ALIAS  a4, r14\n"
    "   16: GOTO       L1\n"
    "   17: LABEL      L2\n"
    "   18: SLLI       r15, r8, 2\n"
    "   19: SRAI       r16, r15, 26\n"
    "   20: SLLI       r17, r16, 23\n"
    "   21: LOAD_IMM   r18, 0x3F800000\n"
    "   22: SUB        r19, r18, r17\n"
    "   23: BITCAST    r20, r19\n"
    "   24: ANDI       r21, r9, 1\n"
    "   25: GOTO_IF_NZ r21, L3\n"
    "   26: ANDI       r22, r9, 2\n"
    "   27: GOTO_IF_NZ r22, L4\n"
    "   28: LOAD_U8    r23, 0(r7)\n"
    "   29: LOAD_U8    r24, 1(r7)\n"
    "   30: FSCAST     r25, r23\n"
    "   31: FMUL       r26, r25, r20\n"
    "   32: FSCAST     r27, r24\n"
    "   33: FMUL       r28, r27, r20\n"
    "   34: VBUILD2    r29, r26, r28\n"
    "   35: VFCVT      r30, r29\n"
    "   36: SET_ALIAS  a4, r30\n"
    "   37: GOTO       L1\n"
    "   38: LABEL      L4\n"
    "   39: LOAD_S8    r31, 0(r7)\n"
    "   40: LOAD_S8    r32, 1(r7)\n"
    "   41: FSCAST     r33, r31\n"
    "   42: FMUL       r34, r33, r20\n"
    "   43: FSCAST     r35, r32\n"
    "   44: FMUL       r36, r35, r20\n"
    "   45: VBUILD2    r37, r34, r36\n"
    "   46: VFCVT      r38, r37\n"
    "   47: SET_ALIAS  a4, r38\n"
    "   48: GOTO       L1\n"
    "   49: LABEL      L3\n"
    "   50: ANDI       r39, r9, 2\n"
    "   51: GOTO_IF_NZ r39, L5\n"
    "   52: LOAD_U16_BR r40, 0(r7)\n"
    "   53: LOAD_U16_BR r41, 2(r7)\n"
    "   54: FSCAST     r42, r40\n"
    "   55: FMUL       r43, r42, r20\n"
    "   56: FSCAST     r44, r41\n"
    "   57: FMUL       r45, r44, r20\n"
    "   58: VBUILD2    r46, r43, r45\n"
    "   59: VFCVT      r47, r46\n"
    "   60: SET_ALIAS  a4, r47\n"
    "   61: GOTO       L1\n"
    "   62: LABEL      L5\n"
    "   63: LOAD_S16_BR r48, 0(r7)\n"
    "   64: LOAD_S16_BR r49, 2(r7)\n"
    "   65: FSCAST     r50, r48\n"
    "   66: FMUL       r51, r50, r20\n"
    "   67: FSCAST     r52, r49\n"
    "   68: FMUL       r53, r52, r20\n"
    "   69: VBUILD2    r54, r51, r53\n"
    "   70: VFCVT      r55, r54\n"
    "   71: SET_ALIAS  a4, r55\n"
    "   72: LABEL      L1\n"
    "   73: LOAD_IMM   r56, 4\n"
    "   74: SET_ALIAS  a1, r56\n"
    "   75: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> 1,2\n"
    "Block 1: 0 --> [11,16] --> 9\n"
    "Block 2: 0 --> [17,25] --> 3,6\n"
    "Block 3: 2 --> [26,27] --> 4,5\n"
    "Block 4: 3 --> [28,37] --> 9\n"
    "Block 5: 3 --> [38,48] --> 9\n"
    "Block 6: 2 --> [49,51] --> 7,8\n"
    "Block 7: 6 --> [52,61] --> 9\n"
    "Block 8: 6 --> [62,71] --> 9\n"
    "Block 9: 8,1,4,5,7 --> [72,75] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
