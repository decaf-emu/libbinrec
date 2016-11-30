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
    0xEC,0x20,0x10,0x30,  // fres f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FCAST      r4, r3\n"
    "    4: BITCAST    r5, r4\n"
    "    5: ANDI       r6, r5, -2147483648\n"
    "    6: BFEXT      r7, r5, 0, 23\n"
    "    7: SET_ALIAS  a6, r7\n"
    "    8: BFEXT      r8, r5, 23, 8\n"
    "    9: SET_ALIAS  a5, r8\n"
    "   10: GOTO_IF_Z  r8, L2\n"
    "   11: SEQI       r9, r8, 255\n"
    "   12: GOTO_IF_NZ r9, L3\n"
    "   13: GOTO       L4\n"
    "   14: LABEL      L3\n"
    "   15: GOTO_IF_NZ r7, L5\n"
    "   16: BITCAST    r10, r6\n"
    "   17: SET_ALIAS  a4, r10\n"
    "   18: GOTO       L1\n"
    "   19: LABEL      L5\n"
    "   20: ORI        r11, r5, 4194304\n"
    "   21: BITCAST    r12, r11\n"
    "   22: SET_ALIAS  a4, r12\n"
    "   23: GOTO       L1\n"
    "   24: LABEL      L2\n"
    "   25: GOTO_IF_NZ r7, L6\n"
    "   26: ORI        r13, r6, 2139095040\n"
    "   27: BITCAST    r14, r13\n"
    "   28: SET_ALIAS  a4, r14\n"
    "   29: GOTO       L1\n"
    "   30: LABEL      L6\n"
    "   31: SLTUI      r15, r7, 2097152\n"
    "   32: GOTO_IF_Z  r15, L7\n"
    "   33: ORI        r16, r6, 2139095039\n"
    "   34: BITCAST    r17, r16\n"
    "   35: SET_ALIAS  a4, r17\n"
    "   36: GOTO       L1\n"
    "   37: LABEL      L7\n"
    "   38: SLLI       r18, r7, 1\n"
    "   39: SET_ALIAS  a6, r18\n"
    "   40: ANDI       r19, r18, 8388608\n"
    "   41: GOTO_IF_NZ r19, L8\n"
    "   42: LOAD_IMM   r20, -1\n"
    "   43: SET_ALIAS  a5, r20\n"
    "   44: SLLI       r21, r18, 1\n"
    "   45: SET_ALIAS  a6, r21\n"
    "   46: LABEL      L8\n"
    "   47: GET_ALIAS  r22, a6\n"
    "   48: ANDI       r23, r22, 8388607\n"
    "   49: SET_ALIAS  a6, r23\n"
    "   50: LABEL      L4\n"
    "   51: LOAD       r24, 1008(r1)\n"
    "   52: GET_ALIAS  r25, a6\n"
    "   53: SRLI       r26, r25, 18\n"
    "   54: SLLI       r27, r26, 2\n"
    "   55: LOAD_IMM   r28, 253\n"
    "   56: GET_ALIAS  r29, a5\n"
    "   57: SUB        r30, r28, r29\n"
    "   58: SET_ALIAS  a5, r30\n"
    "   59: ZCAST      r31, r27\n"
    "   60: ADD        r32, r24, r31\n"
    "   61: LOAD_U16   r33, 2(r32)\n"
    "   62: LOAD_U16   r34, 0(r32)\n"
    "   63: BFEXT      r35, r25, 8, 10\n"
    "   64: MUL        r36, r35, r33\n"
    "   65: SLLI       r37, r34, 10\n"
    "   66: SUB        r38, r37, r36\n"
    "   67: SRLI       r39, r38, 1\n"
    "   68: SET_ALIAS  a6, r39\n"
    "   69: GOTO_IF_Z  r30, L9\n"
    "   70: SLTSI      r40, r30, 0\n"
    "   71: GOTO_IF_NZ r40, L10\n"
    "   72: LABEL      L11\n"
    "   73: GET_ALIAS  r41, a6\n"
    "   74: OR         r42, r41, r6\n"
    "   75: GET_ALIAS  r43, a5\n"
    "   76: SLLI       r44, r43, 23\n"
    "   77: OR         r45, r42, r44\n"
    "   78: BITCAST    r46, r45\n"
    "   79: SET_ALIAS  a4, r46\n"
    "   80: GOTO       L1\n"
    "   81: LABEL      L10\n"
    "   82: LOAD_IMM   r47, 0\n"
    "   83: SET_ALIAS  a5, r47\n"
    "   84: GET_ALIAS  r48, a6\n"
    "   85: ORI        r49, r48, 8388608\n"
    "   86: SRLI       r50, r49, 2\n"
    "   87: SET_ALIAS  a6, r50\n"
    "   88: GOTO       L11\n"
    "   89: LABEL      L9\n"
    "   90: GET_ALIAS  r51, a6\n"
    "   91: ORI        r52, r51, 8388608\n"
    "   92: SRLI       r53, r52, 1\n"
    "   93: SET_ALIAS  a6, r53\n"
    "   94: GOTO       L11\n"
    "   95: LABEL      L1\n"
    "   96: GET_ALIAS  r54, a4\n"
    "   97: FCAST      r55, r54\n"
    "   98: STORE      408(r1), r55\n"
    "   99: SET_ALIAS  a2, r55\n"
    "  100: LOAD_IMM   r56, 4\n"
    "  101: SET_ALIAS  a1, r56\n"
    "  102: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,10] --> 1,6\n"
    "Block 1: 0 --> [11,12] --> 2,3\n"
    "Block 2: 1 --> [13,13] --> 13\n"
    "Block 3: 1 --> [14,15] --> 4,5\n"
    "Block 4: 3 --> [16,18] --> 18\n"
    "Block 5: 3 --> [19,23] --> 18\n"
    "Block 6: 0 --> [24,25] --> 7,8\n"
    "Block 7: 6 --> [26,29] --> 18\n"
    "Block 8: 6 --> [30,32] --> 9,10\n"
    "Block 9: 8 --> [33,36] --> 18\n"
    "Block 10: 8 --> [37,41] --> 11,12\n"
    "Block 11: 10 --> [42,45] --> 12\n"
    "Block 12: 11,10 --> [46,49] --> 13\n"
    "Block 13: 12,2 --> [50,69] --> 14,17\n"
    "Block 14: 13 --> [70,71] --> 15,16\n"
    "Block 15: 14,16,17 --> [72,80] --> 18\n"
    "Block 16: 14 --> [81,88] --> 15\n"
    "Block 17: 13 --> [89,94] --> 15\n"
    "Block 18: 4,5,7,9,15 --> [95,102] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
