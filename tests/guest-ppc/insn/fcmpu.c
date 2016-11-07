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
    0xFF,0x81,0x10,0x00,  // fcmpu cr7,f1,f2
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
    "    7: FCMP       r7, r5, r6, LT\n"
    "    8: FCMP       r8, r5, r6, GT\n"
    "    9: FCMP       r9, r5, r6, EQ\n"
    "   10: FCMP       r10, r5, r6, UN\n"
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
    "   30: NOT        r23, r22\n"
    "   31: ORI        r24, r22, 16777216\n"
    "   32: ANDI       r25, r23, 16777216\n"
    "   33: SET_ALIAS  a9, r24\n"
    "   34: GOTO_IF_Z  r25, L2\n"
    "   35: ORI        r26, r24, -2147483648\n"
    "   36: SET_ALIAS  a9, r26\n"
    "   37: LABEL      L2\n"
    "   38: LABEL      L1\n"
    "   39: LOAD_IMM   r27, 4\n"
    "   40: SET_ALIAS  a1, r27\n"
    "   41: GET_ALIAS  r28, a8\n"
    "   42: ANDI       r29, r28, -16\n"
    "   43: GET_ALIAS  r30, a4\n"
    "   44: SLLI       r31, r30, 3\n"
    "   45: OR         r32, r29, r31\n"
    "   46: GET_ALIAS  r33, a5\n"
    "   47: SLLI       r34, r33, 2\n"
    "   48: OR         r35, r32, r34\n"
    "   49: GET_ALIAS  r36, a6\n"
    "   50: SLLI       r37, r36, 1\n"
    "   51: OR         r38, r35, r37\n"
    "   52: GET_ALIAS  r39, a7\n"
    "   53: OR         r40, r38, r39\n"
    "   54: SET_ALIAS  a8, r40\n"
    "   55: GET_ALIAS  r41, a9\n"
    "   56: GET_ALIAS  r42, a10\n"
    "   57: ANDI       r43, r41, -1611134977\n"
    "   58: SLLI       r44, r42, 12\n"
    "   59: OR         r45, r43, r44\n"
    "   60: SET_ALIAS  a9, r45\n"
    "   61: RETURN\n"
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
    "Block 1: 0 --> [28,34] --> 2,3\n"
    "Block 2: 1 --> [35,36] --> 3\n"
    "Block 3: 2,1 --> [37,37] --> 4\n"
    "Block 4: 3,0 --> [38,61] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
