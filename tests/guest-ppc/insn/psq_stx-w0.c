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

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN
                                    | BINREC_OPT_G_PPC_FAST_STFS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: GET_ALIAS  r4, a2\n"
    "    4: GET_ALIAS  r5, a3\n"
    "    5: ADD        r6, r4, r5\n"
    "    6: ZCAST      r7, r6\n"
    "    7: ADD        r8, r2, r7\n"
    "    8: LOAD       r9, 904(r1)\n"
    "    9: BFEXT      r10, r9, 0, 3\n"
    "   10: ANDI       r11, r10, 4\n"
    "   11: GOTO_IF_NZ r11, L2\n"
    "   12: FGETSTATE  r12\n"
    "   13: VFCVT      r13, r3\n"
    "   14: FSETSTATE  r12\n"
    "   15: VEXTRACT   r14, r13, 0\n"
    "   16: BITCAST    r15, r14\n"
    "   17: BFEXT      r16, r15, 23, 8\n"
    "   18: ANDI       r17, r15, -2147483648\n"
    "   19: BITCAST    r18, r17\n"
    "   20: SELECT     r19, r14, r18, r16\n"
    "   21: STORE_BR   0(r8), r19\n"
    "   22: VEXTRACT   r20, r13, 1\n"
    "   23: BITCAST    r21, r20\n"
    "   24: BFEXT      r22, r21, 23, 8\n"
    "   25: ANDI       r23, r21, -2147483648\n"
    "   26: BITCAST    r24, r23\n"
    "   27: SELECT     r25, r20, r24, r22\n"
    "   28: STORE_BR   4(r8), r25\n"
    "   29: GOTO       L1\n"
    "   30: LABEL      L2\n"
    "   31: SLLI       r26, r9, 18\n"
    "   32: SRAI       r27, r26, 26\n"
    "   33: SLLI       r28, r27, 23\n"
    "   34: ADDI       r29, r28, 1065353216\n"
    "   35: BITCAST    r30, r29\n"
    "   36: ANDI       r31, r10, 2\n"
    "   37: SLLI       r32, r31, 14\n"
    "   38: ANDI       r33, r10, 1\n"
    "   39: XORI       r34, r33, 1\n"
    "   40: SLLI       r35, r34, 3\n"
    "   41: LOAD_IMM   r36, 0\n"
    "   42: LOAD_IMM   r37, 65535\n"
    "   43: SUB        r38, r36, r32\n"
    "   44: SUB        r39, r37, r32\n"
    "   45: SRA        r40, r38, r35\n"
    "   46: SRA        r41, r39, r35\n"
    "   47: FGETSTATE  r42\n"
    "   48: VFCVT      r43, r3\n"
    "   49: VEXTRACT   r44, r43, 0\n"
    "   50: FMUL       r45, r44, r30\n"
    "   51: BITCAST    r46, r45\n"
    "   52: FTRUNCI    r47, r45\n"
    "   53: SLLI       r48, r46, 1\n"
    "   54: SRLI       r49, r46, 31\n"
    "   55: SELECT     r50, r40, r41, r49\n"
    "   56: SGTUI      r51, r48, -1895825409\n"
    "   57: SELECT     r52, r50, r47, r51\n"
    "   58: SGTS       r53, r52, r41\n"
    "   59: SELECT     r54, r41, r52, r53\n"
    "   60: SLTS       r55, r52, r40\n"
    "   61: SELECT     r56, r40, r54, r55\n"
    "   62: VEXTRACT   r57, r43, 1\n"
    "   63: FMUL       r58, r57, r30\n"
    "   64: BITCAST    r59, r58\n"
    "   65: FTRUNCI    r60, r58\n"
    "   66: SLLI       r61, r59, 1\n"
    "   67: SRLI       r62, r59, 31\n"
    "   68: SELECT     r63, r40, r41, r62\n"
    "   69: SGTUI      r64, r61, -1895825409\n"
    "   70: SELECT     r65, r63, r60, r64\n"
    "   71: SGTS       r66, r65, r41\n"
    "   72: SELECT     r67, r41, r65, r66\n"
    "   73: SLTS       r68, r65, r40\n"
    "   74: SELECT     r69, r40, r67, r68\n"
    "   75: FSETSTATE  r42\n"
    "   76: ANDI       r70, r10, 1\n"
    "   77: GOTO_IF_NZ r70, L3\n"
    "   78: STORE_I8   0(r8), r56\n"
    "   79: STORE_I8   1(r8), r69\n"
    "   80: GOTO       L1\n"
    "   81: LABEL      L3\n"
    "   82: STORE_I16_BR 0(r8), r56\n"
    "   83: STORE_I16_BR 2(r8), r69\n"
    "   84: LABEL      L1\n"
    "   85: LOAD_IMM   r71, 4\n"
    "   86: SET_ALIAS  a1, r71\n"
    "   87: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,11] --> 1,2\n"
    "Block 1: 0 --> [12,29] --> 5\n"
    "Block 2: 0 --> [30,77] --> 3,4\n"
    "Block 3: 2 --> [78,80] --> 5\n"
    "Block 4: 2 --> [81,83] --> 5\n"
    "Block 5: 4,1,3 --> [84,87] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
