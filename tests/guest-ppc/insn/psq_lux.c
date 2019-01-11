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
    0x10,0x23,0x25,0x4C,  // psq_lux f1,r3,r4,1,2
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
    "   12: LOAD_IMM   r12, 1.0f\n"
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
    "   29: FSCAST     r24, r23\n"
    "   30: FMUL       r25, r24, r20\n"
    "   31: LOAD_IMM   r26, 1.0f\n"
    "   32: VBUILD2    r27, r25, r26\n"
    "   33: VFCVT      r28, r27\n"
    "   34: SET_ALIAS  a4, r28\n"
    "   35: GOTO       L1\n"
    "   36: LABEL      L4\n"
    "   37: LOAD_S8    r29, 0(r7)\n"
    "   38: FSCAST     r30, r29\n"
    "   39: FMUL       r31, r30, r20\n"
    "   40: LOAD_IMM   r32, 1.0f\n"
    "   41: VBUILD2    r33, r31, r32\n"
    "   42: VFCVT      r34, r33\n"
    "   43: SET_ALIAS  a4, r34\n"
    "   44: GOTO       L1\n"
    "   45: LABEL      L3\n"
    "   46: ANDI       r35, r9, 2\n"
    "   47: GOTO_IF_NZ r35, L5\n"
    "   48: LOAD_U16_BR r36, 0(r7)\n"
    "   49: FSCAST     r37, r36\n"
    "   50: FMUL       r38, r37, r20\n"
    "   51: LOAD_IMM   r39, 1.0f\n"
    "   52: VBUILD2    r40, r38, r39\n"
    "   53: VFCVT      r41, r40\n"
    "   54: SET_ALIAS  a4, r41\n"
    "   55: GOTO       L1\n"
    "   56: LABEL      L5\n"
    "   57: LOAD_S16_BR r42, 0(r7)\n"
    "   58: FSCAST     r43, r42\n"
    "   59: FMUL       r44, r43, r20\n"
    "   60: LOAD_IMM   r45, 1.0f\n"
    "   61: VBUILD2    r46, r44, r45\n"
    "   62: VFCVT      r47, r46\n"
    "   63: SET_ALIAS  a4, r47\n"
    "   64: LABEL      L1\n"
    "   65: SET_ALIAS  a2, r5\n"
    "   66: LOAD_IMM   r48, 4\n"
    "   67: SET_ALIAS  a1, r48\n"
    "   68: RETURN     r1\n"
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
    "Block 4: 3 --> [28,35] --> 9\n"
    "Block 5: 3 --> [36,44] --> 9\n"
    "Block 6: 2 --> [45,47] --> 7,8\n"
    "Block 7: 6 --> [48,55] --> 9\n"
    "Block 8: 6 --> [56,63] --> 9\n"
    "Block 9: 8,1,4,5,7 --> [64,68] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
