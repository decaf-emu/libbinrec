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
    0xFF,0x84,0x00,0x80,  // mcrfs cr7,cr1
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a7\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a8, r4\n"
    "    5: GET_ALIAS  r5, a7\n"
    "    6: BFEXT      r6, r5, 27, 1\n"
    "    7: BFEXT      r7, r5, 26, 1\n"
    "    8: BFEXT      r8, r5, 25, 1\n"
    "    9: BFEXT      r9, r5, 24, 1\n"
    "   10: SET_ALIAS  a2, r6\n"
    "   11: SET_ALIAS  a3, r7\n"
    "   12: SET_ALIAS  a4, r8\n"
    "   13: SET_ALIAS  a5, r9\n"
    "   14: ANDI       r10, r5, -1862270977\n"
    "   15: ANDI       r11, r10, 33031936\n"
    "   16: SGTUI      r12, r11, 0\n"
    "   17: SLLI       r13, r12, 29\n"
    "   18: OR         r14, r10, r13\n"
    "   19: SRLI       r15, r14, 25\n"
    "   20: SRLI       r16, r14, 3\n"
    "   21: AND        r17, r15, r16\n"
    "   22: ANDI       r18, r17, 31\n"
    "   23: SGTUI      r19, r18, 0\n"
    "   24: SLLI       r20, r19, 30\n"
    "   25: OR         r21, r14, r20\n"
    "   26: SET_ALIAS  a7, r21\n"
    "   27: LOAD_IMM   r22, 4\n"
    "   28: SET_ALIAS  a1, r22\n"
    "   29: GET_ALIAS  r23, a6\n"
    "   30: ANDI       r24, r23, -16\n"
    "   31: GET_ALIAS  r25, a2\n"
    "   32: SLLI       r26, r25, 3\n"
    "   33: OR         r27, r24, r26\n"
    "   34: GET_ALIAS  r28, a3\n"
    "   35: SLLI       r29, r28, 2\n"
    "   36: OR         r30, r27, r29\n"
    "   37: GET_ALIAS  r31, a4\n"
    "   38: SLLI       r32, r31, 1\n"
    "   39: OR         r33, r30, r32\n"
    "   40: GET_ALIAS  r34, a5\n"
    "   41: OR         r35, r33, r34\n"
    "   42: SET_ALIAS  a6, r35\n"
    "   43: GET_ALIAS  r36, a7\n"
    "   44: GET_ALIAS  r37, a8\n"
    "   45: ANDI       r38, r36, -522241\n"
    "   46: SLLI       r39, r37, 12\n"
    "   47: OR         r40, r38, r39\n"
    "   48: SET_ALIAS  a7, r40\n"
    "   49: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32 @ 944(r1)\n"
    "Alias 8: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,49] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
