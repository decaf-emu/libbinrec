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

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN
                                    | BINREC_OPT_G_PPC_FAST_STFS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: FGETSTATE  r4\n"
    "    4: VFCVT      r5, r3\n"
    "    5: FSETSTATE  r4\n"
    "    6: GET_ALIAS  r6, a2\n"
    "    7: ZCAST      r7, r6\n"
    "    8: ADD        r8, r2, r7\n"
    "    9: LOAD       r9, 904(r1)\n"
    "   10: BFEXT      r10, r9, 0, 3\n"
    "   11: ANDI       r11, r10, 4\n"
    "   12: GOTO_IF_NZ r11, L2\n"
    "   13: VEXTRACT   r12, r5, 0\n"
    "   14: BITCAST    r13, r12\n"
    "   15: BFEXT      r14, r13, 23, 8\n"
    "   16: ANDI       r15, r13, -2147483648\n"
    "   17: BITCAST    r16, r15\n"
    "   18: SELECT     r17, r12, r16, r14\n"
    "   19: STORE_BR   -16(r8), r17\n"
    "   20: GOTO       L1\n"
    "   21: LABEL      L2\n"
    "   22: SLLI       r18, r9, 18\n"
    "   23: SRAI       r19, r18, 26\n"
    "   24: SLLI       r20, r19, 23\n"
    "   25: ADDI       r21, r20, 1065353216\n"
    "   26: BITCAST    r22, r21\n"
    "   27: ANDI       r23, r10, 2\n"
    "   28: SLLI       r24, r23, 14\n"
    "   29: ANDI       r25, r10, 1\n"
    "   30: XORI       r26, r25, 1\n"
    "   31: SLLI       r27, r26, 3\n"
    "   32: LOAD_IMM   r28, 0\n"
    "   33: LOAD_IMM   r29, 65535\n"
    "   34: SUB        r30, r28, r24\n"
    "   35: SUB        r31, r29, r24\n"
    "   36: SRA        r32, r30, r27\n"
    "   37: SRA        r33, r31, r27\n"
    "   38: FGETSTATE  r34\n"
    "   39: VEXTRACT   r35, r5, 0\n"
    "   40: FMUL       r36, r35, r22\n"
    "   41: BITCAST    r37, r36\n"
    "   42: FTRUNCI    r38, r36\n"
    "   43: SLLI       r39, r37, 1\n"
    "   44: SRLI       r40, r37, 31\n"
    "   45: SELECT     r41, r32, r33, r40\n"
    "   46: SGTUI      r42, r39, -1895825409\n"
    "   47: SELECT     r43, r41, r38, r42\n"
    "   48: SGTS       r44, r43, r33\n"
    "   49: SELECT     r45, r33, r43, r44\n"
    "   50: SLTS       r46, r43, r32\n"
    "   51: SELECT     r47, r32, r45, r46\n"
    "   52: FSETSTATE  r34\n"
    "   53: ANDI       r48, r10, 1\n"
    "   54: GOTO_IF_NZ r48, L3\n"
    "   55: STORE_I8   -16(r8), r47\n"
    "   56: GOTO       L1\n"
    "   57: LABEL      L3\n"
    "   58: STORE_I16_BR -16(r8), r47\n"
    "   59: LABEL      L1\n"
    "   60: VFCVT      r49, r5\n"
    "   61: SET_ALIAS  a3, r49\n"
    "   62: LOAD_IMM   r50, 8\n"
    "   63: SET_ALIAS  a1, r50\n"
    "   64: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "Alias 4: float64[2] @ 416(r1)\n"
    "\n"
    "Block 0: <none> --> [0,12] --> 1,2\n"
    "Block 1: 0 --> [13,20] --> 5\n"
    "Block 2: 0 --> [21,54] --> 3,4\n"
    "Block 3: 2 --> [55,56] --> 5\n"
    "Block 4: 2 --> [57,58] --> 5\n"
    "Block 5: 4,1,3 --> [59,64] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
