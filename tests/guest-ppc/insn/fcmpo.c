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
    0xFF,0x81,0x10,0x40,  // fcmpo cr7,f1,f2
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a9\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a10, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: GET_ALIAS  r6, a3\n"
    "    7: FCMP       r7, r5, r6, OLT\n"
    "    8: FCMP       r8, r5, r6, OGT\n"
    "    9: FCMP       r9, r5, r6, OEQ\n"
    "   10: FCMP       r10, r5, r6, OUN\n"
    "   11: SET_ALIAS  a4, r7\n"
    "   12: SET_ALIAS  a5, r8\n"
    "   13: SET_ALIAS  a6, r9\n"
    "   14: SET_ALIAS  a7, r10\n"
    "   15: GET_ALIAS  r11, a10\n"
    "   16: ANDI       r12, r11, 112\n"
    "   17: SLLI       r13, r7, 3\n"
    "   18: SLLI       r14, r8, 2\n"
    "   19: SLLI       r15, r9, 1\n"
    "   20: OR         r16, r13, r14\n"
    "   21: OR         r17, r15, r10\n"
    "   22: OR         r18, r16, r17\n"
    "   23: OR         r19, r12, r18\n"
    "   24: SET_ALIAS  a10, r19\n"
    "   25: FGETSTATE  r20\n"
    "   26: FTESTEXC   r21, r20, INVALID\n"
    "   27: GOTO_IF_Z  r21, L1\n"
    "   28: FCLEAREXC\n"
    "   29: GET_ALIAS  r22, a9\n"
    "   30: BITCAST    r23, r5\n"
    "   31: BITCAST    r24, r6\n"
    "   32: AND        r25, r23, r24\n"
    "   33: SRLI       r26, r25, 51\n"
    "   34: ZCAST      r27, r26\n"
    "   35: ANDI       r28, r27, 4095\n"
    "   36: SEQI       r29, r28, 4095\n"
    "   37: GOTO_IF_Z  r29, L2\n"
    "   38: ORI        r30, r22, 537395200\n"
    "   39: SET_ALIAS  a9, r30\n"
    "   40: GOTO       L1\n"
    "   41: LABEL      L2\n"
    "   42: ORI        r31, r22, 553648128\n"
    "   43: SET_ALIAS  a9, r31\n"
    "   44: LABEL      L1\n"
    "   45: LOAD_IMM   r32, 4\n"
    "   46: SET_ALIAS  a1, r32\n"
    "   47: GET_ALIAS  r33, a8\n"
    "   48: ANDI       r34, r33, -16\n"
    "   49: GET_ALIAS  r35, a4\n"
    "   50: SLLI       r36, r35, 3\n"
    "   51: OR         r37, r34, r36\n"
    "   52: GET_ALIAS  r38, a5\n"
    "   53: SLLI       r39, r38, 2\n"
    "   54: OR         r40, r37, r39\n"
    "   55: GET_ALIAS  r41, a6\n"
    "   56: SLLI       r42, r41, 1\n"
    "   57: OR         r43, r40, r42\n"
    "   58: GET_ALIAS  r44, a7\n"
    "   59: OR         r45, r43, r44\n"
    "   60: SET_ALIAS  a8, r45\n"
    "   61: GET_ALIAS  r46, a9\n"
    "   62: GET_ALIAS  r47, a10\n"
    "   63: ANDI       r48, r46, -522241\n"
    "   64: SLLI       r49, r47, 12\n"
    "   65: OR         r50, r48, r49\n"
    "   66: SET_ALIAS  a9, r50\n"
    "   67: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 928(r1)\n"
    "Alias 9: int32 @ 944(r1)\n"
    "Alias 10: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,27] --> 1,4\n"
    "Block 1: 0 --> [28,37] --> 2,3\n"
    "Block 2: 1 --> [38,40] --> 4\n"
    "Block 3: 1 --> [41,43] --> 4\n"
    "Block 4: 3,0,2 --> [44,67] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
