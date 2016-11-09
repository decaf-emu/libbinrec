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
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 3, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: BFEXT      r5, r3, 2, 1\n"
    "    6: SET_ALIAS  a4, r5\n"
    "    7: BFEXT      r6, r3, 1, 1\n"
    "    8: SET_ALIAS  a5, r6\n"
    "    9: BFEXT      r7, r3, 0, 1\n"
    "   10: SET_ALIAS  a6, r7\n"
    "   11: GET_ALIAS  r8, a7\n"
    "   12: BFEXT      r9, r8, 12, 7\n"
    "   13: SET_ALIAS  a8, r9\n"
    "   14: GET_ALIAS  r10, a7\n"
    "   15: GET_ALIAS  r11, a8\n"
    "   16: BFEXT      r12, r10, 19, 1\n"
    "   17: BFEXT      r13, r11, 6, 1\n"
    "   18: BFEXT      r14, r11, 5, 1\n"
    "   19: BFEXT      r15, r11, 4, 1\n"
    "   20: SET_ALIAS  a3, r12\n"
    "   21: SET_ALIAS  a4, r13\n"
    "   22: SET_ALIAS  a5, r14\n"
    "   23: SET_ALIAS  a6, r15\n"
    "   24: ANDI       r16, r10, -524289\n"
    "   25: SET_ALIAS  a7, r16\n"
    "   26: LOAD_IMM   r17, 4\n"
    "   27: SET_ALIAS  a1, r17\n"
    "   28: GET_ALIAS  r18, a2\n"
    "   29: ANDI       r19, r18, -16\n"
    "   30: GET_ALIAS  r20, a3\n"
    "   31: SLLI       r21, r20, 3\n"
    "   32: OR         r22, r19, r21\n"
    "   33: GET_ALIAS  r23, a4\n"
    "   34: SLLI       r24, r23, 2\n"
    "   35: OR         r25, r22, r24\n"
    "   36: GET_ALIAS  r26, a5\n"
    "   37: SLLI       r27, r26, 1\n"
    "   38: OR         r28, r25, r27\n"
    "   39: GET_ALIAS  r29, a6\n"
    "   40: OR         r30, r28, r29\n"
    "   41: SET_ALIAS  a2, r30\n"
    "   42: GET_ALIAS  r31, a7\n"
    "   43: GET_ALIAS  r32, a8\n"
    "   44: ANDI       r33, r31, -1611134977\n"
    "   45: SLLI       r34, r32, 12\n"
    "   46: OR         r35, r33, r34\n"
    "   47: SET_ALIAS  a7, r35\n"
    "   48: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32 @ 944(r1)\n"
    "Alias 8: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,48] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
