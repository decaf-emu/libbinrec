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
    0xFC,0x20,0x10,0x1C,  // fctiw f1,f2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FROUNDI    r4, r3\n"
    "    4: LOAD_IMM   r5, 2147483647.0\n"
    "    5: FCMP       r6, r3, r5, GE\n"
    "    6: LOAD_IMM   r7, 0x7FFFFFFF\n"
    "    7: SELECT     r8, r7, r4, r6\n"
    "    8: ZCAST      r9, r8\n"
    "    9: BITCAST    r10, r3\n"
    "   10: LOAD_IMM   r11, 0x8000000000000000\n"
    "   11: SEQ        r12, r10, r11\n"
    "   12: SLLI       r13, r12, 32\n"
    "   13: LOAD_IMM   r14, 0xFFF8000000000000\n"
    "   14: OR         r15, r14, r13\n"
    "   15: OR         r16, r9, r15\n"
    "   16: BITCAST    r17, r16\n"
    "   17: GET_ALIAS  r18, a4\n"
    "   18: FGETSTATE  r19\n"
    "   19: FCLEAREXC  r20, r19\n"
    "   20: FSETSTATE  r20\n"
    "   21: FTESTEXC   r21, r19, INVALID\n"
    "   22: GOTO_IF_Z  r21, L2\n"
    "   23: BITCAST    r22, r3\n"
    "   24: SLLI       r23, r22, 13\n"
    "   25: BFEXT      r24, r22, 51, 12\n"
    "   26: SEQI       r25, r24, 4094\n"
    "   27: GOTO_IF_Z  r25, L4\n"
    "   28: GOTO_IF_NZ r23, L3\n"
    "   29: LABEL      L4\n"
    "   30: NOT        r26, r18\n"
    "   31: ORI        r27, r18, 256\n"
    "   32: ANDI       r28, r26, 256\n"
    "   33: SET_ALIAS  a4, r27\n"
    "   34: GOTO_IF_Z  r28, L5\n"
    "   35: ORI        r29, r27, -2147483648\n"
    "   36: SET_ALIAS  a4, r29\n"
    "   37: LABEL      L5\n"
    "   38: GOTO       L6\n"
    "   39: LABEL      L3\n"
    "   40: NOT        r30, r18\n"
    "   41: ORI        r31, r18, 16777472\n"
    "   42: ANDI       r32, r30, 16777472\n"
    "   43: SET_ALIAS  a4, r31\n"
    "   44: GOTO_IF_Z  r32, L7\n"
    "   45: ORI        r33, r31, -2147483648\n"
    "   46: SET_ALIAS  a4, r33\n"
    "   47: LABEL      L7\n"
    "   48: LABEL      L6\n"
    "   49: ANDI       r34, r18, 128\n"
    "   50: GOTO_IF_Z  r34, L2\n"
    "   51: GET_ALIAS  r35, a4\n"
    "   52: BFEXT      r36, r35, 12, 7\n"
    "   53: ANDI       r37, r36, 31\n"
    "   54: GET_ALIAS  r38, a4\n"
    "   55: BFINS      r39, r38, r37, 12, 7\n"
    "   56: SET_ALIAS  a4, r39\n"
    "   57: GOTO       L1\n"
    "   58: LABEL      L2\n"
    "   59: SET_ALIAS  a2, r17\n"
    "   60: BITCAST    r40, r17\n"
    "   61: SGTUI      r41, r40, 0\n"
    "   62: SLTSI      r42, r40, 0\n"
    "   63: BFEXT      r46, r40, 52, 11\n"
    "   64: SEQI       r43, r46, 0\n"
    "   65: SEQI       r44, r46, 2047\n"
    "   66: SLLI       r47, r40, 12\n"
    "   67: SEQI       r45, r47, 0\n"
    "   68: AND        r48, r43, r45\n"
    "   69: XORI       r49, r45, 1\n"
    "   70: AND        r50, r44, r49\n"
    "   71: AND        r51, r43, r41\n"
    "   72: OR         r52, r51, r50\n"
    "   73: OR         r53, r48, r50\n"
    "   74: XORI       r54, r53, 1\n"
    "   75: XORI       r55, r42, 1\n"
    "   76: AND        r56, r42, r54\n"
    "   77: AND        r57, r55, r54\n"
    "   78: SLLI       r58, r52, 4\n"
    "   79: SLLI       r59, r56, 3\n"
    "   80: SLLI       r60, r57, 2\n"
    "   81: SLLI       r61, r48, 1\n"
    "   82: OR         r62, r58, r59\n"
    "   83: OR         r63, r60, r61\n"
    "   84: OR         r64, r62, r44\n"
    "   85: OR         r65, r64, r63\n"
    "   86: FTESTEXC   r66, r19, INEXACT\n"
    "   87: SLLI       r67, r66, 5\n"
    "   88: OR         r68, r65, r67\n"
    "   89: GET_ALIAS  r69, a4\n"
    "   90: BFINS      r70, r69, r68, 12, 7\n"
    "   91: SET_ALIAS  a4, r70\n"
    "   92: GOTO_IF_Z  r66, L8\n"
    "   93: GET_ALIAS  r71, a4\n"
    "   94: NOT        r72, r71\n"
    "   95: ORI        r73, r71, 33554432\n"
    "   96: ANDI       r74, r72, 33554432\n"
    "   97: SET_ALIAS  a4, r73\n"
    "   98: GOTO_IF_Z  r74, L9\n"
    "   99: ORI        r75, r73, -2147483648\n"
    "  100: SET_ALIAS  a4, r75\n"
    "  101: LABEL      L9\n"
    "  102: LABEL      L8\n"
    "  103: LABEL      L1\n"
    "  104: LOAD_IMM   r76, 4\n"
    "  105: SET_ALIAS  a1, r76\n"
    "  106: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,22] --> 1,11\n"
    "Block 1: 0 --> [23,27] --> 2,3\n"
    "Block 2: 1 --> [28,28] --> 3,6\n"
    "Block 3: 2,1 --> [29,34] --> 4,5\n"
    "Block 4: 3 --> [35,36] --> 5\n"
    "Block 5: 4,3 --> [37,38] --> 9\n"
    "Block 6: 2 --> [39,44] --> 7,8\n"
    "Block 7: 6 --> [45,46] --> 8\n"
    "Block 8: 7,6 --> [47,47] --> 9\n"
    "Block 9: 8,5 --> [48,50] --> 10,11\n"
    "Block 10: 9 --> [51,57] --> 16\n"
    "Block 11: 0,9 --> [58,92] --> 12,15\n"
    "Block 12: 11 --> [93,98] --> 13,14\n"
    "Block 13: 12 --> [99,100] --> 14\n"
    "Block 14: 13,12 --> [101,101] --> 15\n"
    "Block 15: 14,11 --> [102,102] --> 16\n"
    "Block 16: 15,10 --> [103,106] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
