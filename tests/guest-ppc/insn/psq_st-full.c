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
    0xF0,0x23,0x2F,0xF0,  // psq_st f1,-16(r3),0,2
};

static const unsigned int guest_opt = 0;
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
    "   10: VEXTRACT   r10, r3, 0\n"
    "   11: BITCAST    r11, r10\n"
    "   12: SRLI       r12, r11, 32\n"
    "   13: ZCAST      r13, r12\n"
    "   14: SLLI       r14, r11, 1\n"
    "   15: GOTO_IF_Z  r14, L3\n"
    "   16: LOAD_IMM   r15, 0x701FFFFFFFFFFFFF\n"
    "   17: SGTU       r16, r14, r15\n"
    "   18: GOTO_IF_Z  r16, L4\n"
    "   19: LABEL      L3\n"
    "   20: ANDI       r17, r13, -1073741824\n"
    "   21: BFEXT      r18, r11, 29, 30\n"
    "   22: ZCAST      r19, r18\n"
    "   23: OR         r20, r17, r19\n"
    "   24: STORE_BR   -16(r6), r20\n"
    "   25: GOTO       L5\n"
    "   26: LABEL      L4\n"
    "   27: ANDI       r21, r17, -2147483648\n"
    "   28: STORE_BR   -16(r6), r21\n"
    "   29: LABEL      L5\n"
    "   30: VEXTRACT   r22, r3, 1\n"
    "   31: BITCAST    r23, r22\n"
    "   32: SRLI       r24, r23, 32\n"
    "   33: ZCAST      r25, r24\n"
    "   34: SLLI       r26, r23, 1\n"
    "   35: GOTO_IF_Z  r26, L6\n"
    "   36: LOAD_IMM   r27, 0x701FFFFFFFFFFFFF\n"
    "   37: SGTU       r28, r26, r27\n"
    "   38: GOTO_IF_Z  r28, L7\n"
    "   39: LABEL      L6\n"
    "   40: ANDI       r29, r25, -1073741824\n"
    "   41: BFEXT      r30, r23, 29, 30\n"
    "   42: ZCAST      r31, r30\n"
    "   43: OR         r32, r29, r31\n"
    "   44: STORE_BR   -12(r6), r32\n"
    "   45: GOTO       L8\n"
    "   46: LABEL      L7\n"
    "   47: ANDI       r33, r29, -2147483648\n"
    "   48: STORE_BR   -12(r6), r33\n"
    "   49: LABEL      L8\n"
    "   50: GOTO       L1\n"
    "   51: LABEL      L2\n"
    "   52: SLLI       r34, r7, 18\n"
    "   53: SRAI       r35, r34, 26\n"
    "   54: SLLI       r36, r35, 23\n"
    "   55: ADDI       r37, r36, 1065353216\n"
    "   56: BITCAST    r38, r37\n"
    "   57: ANDI       r39, r8, 2\n"
    "   58: SLLI       r40, r39, 14\n"
    "   59: ANDI       r41, r8, 1\n"
    "   60: XORI       r42, r41, 1\n"
    "   61: SLLI       r43, r42, 3\n"
    "   62: LOAD_IMM   r44, 0\n"
    "   63: LOAD_IMM   r45, 65535\n"
    "   64: SUB        r46, r44, r40\n"
    "   65: SUB        r47, r45, r40\n"
    "   66: SRA        r48, r46, r43\n"
    "   67: SRA        r49, r47, r43\n"
    "   68: FGETSTATE  r50\n"
    "   69: VFCVT      r51, r3\n"
    "   70: VEXTRACT   r52, r51, 0\n"
    "   71: FMUL       r53, r52, r38\n"
    "   72: BITCAST    r54, r53\n"
    "   73: FTRUNCI    r55, r53\n"
    "   74: SLLI       r56, r54, 1\n"
    "   75: SRLI       r57, r54, 31\n"
    "   76: SELECT     r58, r48, r49, r57\n"
    "   77: SGTUI      r59, r56, -1895825409\n"
    "   78: SELECT     r60, r58, r55, r59\n"
    "   79: SGTS       r61, r60, r49\n"
    "   80: SELECT     r62, r49, r60, r61\n"
    "   81: SLTS       r63, r60, r48\n"
    "   82: SELECT     r64, r48, r62, r63\n"
    "   83: VEXTRACT   r65, r51, 1\n"
    "   84: FMUL       r66, r65, r38\n"
    "   85: BITCAST    r67, r66\n"
    "   86: FTRUNCI    r68, r66\n"
    "   87: SLLI       r69, r67, 1\n"
    "   88: SRLI       r70, r67, 31\n"
    "   89: SELECT     r71, r48, r49, r70\n"
    "   90: SGTUI      r72, r69, -1895825409\n"
    "   91: SELECT     r73, r71, r68, r72\n"
    "   92: SGTS       r74, r73, r49\n"
    "   93: SELECT     r75, r49, r73, r74\n"
    "   94: SLTS       r76, r73, r48\n"
    "   95: SELECT     r77, r48, r75, r76\n"
    "   96: FSETSTATE  r50\n"
    "   97: ANDI       r78, r8, 1\n"
    "   98: GOTO_IF_NZ r78, L9\n"
    "   99: STORE_I8   -16(r6), r64\n"
    "  100: STORE_I8   -15(r6), r77\n"
    "  101: GOTO       L1\n"
    "  102: LABEL      L9\n"
    "  103: STORE_I16_BR -16(r6), r64\n"
    "  104: STORE_I16_BR -14(r6), r77\n"
    "  105: LABEL      L1\n"
    "  106: LOAD_IMM   r79, 4\n"
    "  107: SET_ALIAS  a1, r79\n"
    "  108: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,9] --> 1,10\n"
    "Block 1: 0 --> [10,15] --> 2,3\n"
    "Block 2: 1 --> [16,18] --> 3,4\n"
    "Block 3: 2,1 --> [19,25] --> 5\n"
    "Block 4: 2 --> [26,28] --> 5\n"
    "Block 5: 4,3 --> [29,35] --> 6,7\n"
    "Block 6: 5 --> [36,38] --> 7,8\n"
    "Block 7: 6,5 --> [39,45] --> 9\n"
    "Block 8: 6 --> [46,48] --> 9\n"
    "Block 9: 8,7 --> [49,50] --> 13\n"
    "Block 10: 0 --> [51,98] --> 11,12\n"
    "Block 11: 10 --> [99,101] --> 13\n"
    "Block 12: 10 --> [102,104] --> 13\n"
    "Block 13: 12,9,11 --> [105,108] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
