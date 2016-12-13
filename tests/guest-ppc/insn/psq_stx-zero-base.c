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
    0x10,0x20,0x1D,0x0E,  // psq_stx f1,0,r3,1,2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN
                                    | BINREC_OPT_G_PPC_FAST_STFS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a2\n"
    "    4: ZCAST      r5, r4\n"
    "    5: ADD        r6, r2, r5\n"
    "    6: LOAD       r7, 904(r1)\n"
    "    7: BFEXT      r8, r7, 0, 3\n"
    "    8: ANDI       r9, r8, 4\n"
    "    9: GOTO_IF_NZ r9, L2\n"
    "   10: FGETSTATE  r10\n"
    "   11: VEXTRACT   r11, r3, 0\n"
    "   12: FCVT       r12, r11\n"
    "   13: FSETSTATE  r10\n"
    "   14: BITCAST    r13, r12\n"
    "   15: BFEXT      r14, r13, 23, 8\n"
    "   16: ANDI       r15, r13, -2147483648\n"
    "   17: BITCAST    r16, r15\n"
    "   18: SELECT     r17, r12, r16, r14\n"
    "   19: STORE_BR   0(r6), r17\n"
    "   20: GOTO       L1\n"
    "   21: LABEL      L2\n"
    "   22: SLLI       r18, r7, 18\n"
    "   23: SRAI       r19, r18, 26\n"
    "   24: SLLI       r20, r19, 23\n"
    "   25: ADDI       r21, r20, 1065353216\n"
    "   26: BITCAST    r22, r21\n"
    "   27: ANDI       r23, r8, 2\n"
    "   28: SLLI       r24, r23, 14\n"
    "   29: ANDI       r25, r8, 1\n"
    "   30: XORI       r26, r25, 1\n"
    "   31: SLLI       r27, r26, 3\n"
    "   32: LOAD_IMM   r28, 0\n"
    "   33: LOAD_IMM   r29, 65535\n"
    "   34: SUB        r30, r28, r24\n"
    "   35: SUB        r31, r29, r24\n"
    "   36: SRA        r32, r30, r27\n"
    "   37: SRA        r33, r31, r27\n"
    "   38: FGETSTATE  r34\n"
    "   39: VEXTRACT   r35, r3, 0\n"
    "   40: FCVT       r36, r35\n"
    "   41: FMUL       r37, r36, r22\n"
    "   42: BITCAST    r38, r37\n"
    "   43: FTRUNCI    r39, r37\n"
    "   44: SLLI       r40, r38, 1\n"
    "   45: SRLI       r41, r38, 31\n"
    "   46: SELECT     r42, r32, r33, r41\n"
    "   47: SGTUI      r43, r40, -1895825409\n"
    "   48: SELECT     r44, r42, r39, r43\n"
    "   49: SGTS       r45, r44, r33\n"
    "   50: SELECT     r46, r33, r44, r45\n"
    "   51: SLTS       r47, r44, r32\n"
    "   52: SELECT     r48, r32, r46, r47\n"
    "   53: FSETSTATE  r34\n"
    "   54: ANDI       r49, r8, 1\n"
    "   55: GOTO_IF_NZ r49, L3\n"
    "   56: STORE_I8   0(r6), r48\n"
    "   57: GOTO       L1\n"
    "   58: LABEL      L3\n"
    "   59: STORE_I16_BR 0(r6), r48\n"
    "   60: LABEL      L1\n"
    "   61: LOAD_IMM   r50, 4\n"
    "   62: SET_ALIAS  a1, r50\n"
    "   63: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,9] --> 1,2\n"
    "Block 1: 0 --> [10,20] --> 5\n"
    "Block 2: 0 --> [21,55] --> 3,4\n"
    "Block 3: 2 --> [56,57] --> 5\n"
    "Block 4: 2 --> [58,59] --> 5\n"
    "Block 5: 4,1,3 --> [60,63] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
