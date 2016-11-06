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
    0xFF,0x8C,0x00,0x80,  // mcrfs cr7,cr3
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
    "    6: GET_ALIAS  r6, a8\n"
    "    7: BFEXT      r7, r5, 19, 1\n"
    "    8: BFEXT      r8, r6, 6, 1\n"
    "    9: BFEXT      r9, r6, 5, 1\n"
    "   10: BFEXT      r10, r6, 4, 1\n"
    "   11: SET_ALIAS  a2, r7\n"
    "   12: SET_ALIAS  a3, r8\n"
    "   13: SET_ALIAS  a4, r9\n"
    "   14: SET_ALIAS  a5, r10\n"
    "   15: ANDI       r11, r5, -1611137025\n"
    "   16: ANDI       r12, r11, 33031936\n"
    "   17: SGTUI      r13, r12, 0\n"
    "   18: SLLI       r14, r13, 29\n"
    "   19: OR         r15, r11, r14\n"
    "   20: SRLI       r16, r15, 25\n"
    "   21: SRLI       r17, r15, 3\n"
    "   22: AND        r18, r16, r17\n"
    "   23: ANDI       r19, r18, 31\n"
    "   24: SGTUI      r20, r19, 0\n"
    "   25: SLLI       r21, r20, 30\n"
    "   26: OR         r22, r15, r21\n"
    "   27: SET_ALIAS  a7, r22\n"
    "   28: LOAD_IMM   r23, 4\n"
    "   29: SET_ALIAS  a1, r23\n"
    "   30: GET_ALIAS  r24, a6\n"
    "   31: ANDI       r25, r24, -16\n"
    "   32: GET_ALIAS  r26, a2\n"
    "   33: SLLI       r27, r26, 3\n"
    "   34: OR         r28, r25, r27\n"
    "   35: GET_ALIAS  r29, a3\n"
    "   36: SLLI       r30, r29, 2\n"
    "   37: OR         r31, r28, r30\n"
    "   38: GET_ALIAS  r32, a4\n"
    "   39: SLLI       r33, r32, 1\n"
    "   40: OR         r34, r31, r33\n"
    "   41: GET_ALIAS  r35, a5\n"
    "   42: OR         r36, r34, r35\n"
    "   43: SET_ALIAS  a6, r36\n"
    "   44: GET_ALIAS  r37, a7\n"
    "   45: GET_ALIAS  r38, a8\n"
    "   46: ANDI       r39, r37, -522241\n"
    "   47: SLLI       r40, r38, 12\n"
    "   48: OR         r41, r39, r40\n"
    "   49: SET_ALIAS  a7, r41\n"
    "   50: RETURN\n"
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
    "Block 0: <none> --> [0,50] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
