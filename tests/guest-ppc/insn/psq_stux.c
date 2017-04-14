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
    0x10,0x23,0x25,0x4E,  // psq_stux f1,r3,r4,1,2
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
    "   13: VEXTRACT   r13, r3, 0\n"
    "   14: FCVT       r14, r13\n"
    "   15: FSETSTATE  r12\n"
    "   16: BITCAST    r15, r14\n"
    "   17: BFEXT      r16, r15, 23, 8\n"
    "   18: ANDI       r17, r15, -2147483648\n"
    "   19: BITCAST    r18, r17\n"
    "   20: SELECT     r19, r14, r18, r16\n"
    "   21: STORE_BR   0(r8), r19\n"
    "   22: GOTO       L1\n"
    "   23: LABEL      L2\n"
    "   24: SLLI       r20, r9, 18\n"
    "   25: SRAI       r21, r20, 26\n"
    "   26: SLLI       r22, r21, 23\n"
    "   27: ADDI       r23, r22, 1065353216\n"
    "   28: BITCAST    r24, r23\n"
    "   29: ANDI       r25, r10, 2\n"
    "   30: SLLI       r26, r25, 14\n"
    "   31: ANDI       r27, r10, 1\n"
    "   32: XORI       r28, r27, 1\n"
    "   33: SLLI       r29, r28, 3\n"
    "   34: LOAD_IMM   r30, 0\n"
    "   35: LOAD_IMM   r31, 65535\n"
    "   36: SUB        r32, r30, r26\n"
    "   37: SUB        r33, r31, r26\n"
    "   38: SRA        r34, r32, r29\n"
    "   39: SRA        r35, r33, r29\n"
    "   40: FGETSTATE  r36\n"
    "   41: VEXTRACT   r37, r3, 0\n"
    "   42: FCVT       r38, r37\n"
    "   43: FMUL       r39, r38, r24\n"
    "   44: BITCAST    r40, r39\n"
    "   45: FTRUNCI    r41, r39\n"
    "   46: SLLI       r42, r40, 1\n"
    "   47: SRLI       r43, r40, 31\n"
    "   48: SELECT     r44, r34, r35, r43\n"
    "   49: SGTUI      r45, r42, -1895825409\n"
    "   50: SELECT     r46, r44, r41, r45\n"
    "   51: SGTS       r47, r46, r35\n"
    "   52: SELECT     r48, r35, r46, r47\n"
    "   53: SLTS       r49, r46, r34\n"
    "   54: SELECT     r50, r34, r48, r49\n"
    "   55: FSETSTATE  r36\n"
    "   56: ANDI       r51, r10, 1\n"
    "   57: GOTO_IF_NZ r51, L3\n"
    "   58: STORE_I8   0(r8), r50\n"
    "   59: GOTO       L1\n"
    "   60: LABEL      L3\n"
    "   61: STORE_I16_BR 0(r8), r50\n"
    "   62: LABEL      L1\n"
    "   63: SET_ALIAS  a2, r6\n"
    "   64: LOAD_IMM   r52, 4\n"
    "   65: SET_ALIAS  a1, r52\n"
    "   66: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,11] --> 1,2\n"
    "Block 1: 0 --> [12,22] --> 5\n"
    "Block 2: 0 --> [23,57] --> 3,4\n"
    "Block 3: 2 --> [58,59] --> 5\n"
    "Block 4: 2 --> [60,61] --> 5\n"
    "Block 5: 4,1,3 --> [62,66] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
