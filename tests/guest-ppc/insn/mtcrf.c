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
    0x7C,0x60,0x51,0x20,  // mtcrf 5,r3
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 11, 1\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: BFEXT      r5, r3, 10, 1\n"
    "    6: SET_ALIAS  a5, r5\n"
    "    7: BFEXT      r6, r3, 9, 1\n"
    "    8: SET_ALIAS  a6, r6\n"
    "    9: BFEXT      r7, r3, 8, 1\n"
    "   10: SET_ALIAS  a7, r7\n"
    "   11: BFEXT      r8, r3, 3, 1\n"
    "   12: SET_ALIAS  a8, r8\n"
    "   13: BFEXT      r9, r3, 2, 1\n"
    "   14: SET_ALIAS  a9, r9\n"
    "   15: BFEXT      r10, r3, 1, 1\n"
    "   16: SET_ALIAS  a10, r10\n"
    "   17: BFEXT      r11, r3, 0, 1\n"
    "   18: SET_ALIAS  a11, r11\n"
    "   19: GET_ALIAS  r12, a2\n"
    "   20: BFEXT      r13, r12, 11, 1\n"
    "   21: BFEXT      r14, r12, 10, 1\n"
    "   22: BFEXT      r15, r12, 9, 1\n"
    "   23: BFEXT      r16, r12, 8, 1\n"
    "   24: SET_ALIAS  a4, r13\n"
    "   25: SET_ALIAS  a5, r14\n"
    "   26: SET_ALIAS  a6, r15\n"
    "   27: SET_ALIAS  a7, r16\n"
    "   28: BFEXT      r17, r12, 3, 1\n"
    "   29: BFEXT      r18, r12, 2, 1\n"
    "   30: BFEXT      r19, r12, 1, 1\n"
    "   31: BFEXT      r20, r12, 0, 1\n"
    "   32: SET_ALIAS  a8, r17\n"
    "   33: SET_ALIAS  a9, r18\n"
    "   34: SET_ALIAS  a10, r19\n"
    "   35: SET_ALIAS  a11, r20\n"
    "   36: LOAD_IMM   r21, 4\n"
    "   37: SET_ALIAS  a1, r21\n"
    "   38: GET_ALIAS  r22, a3\n"
    "   39: ANDI       r23, r22, -3856\n"
    "   40: GET_ALIAS  r24, a4\n"
    "   41: SLLI       r25, r24, 11\n"
    "   42: OR         r26, r23, r25\n"
    "   43: GET_ALIAS  r27, a5\n"
    "   44: SLLI       r28, r27, 10\n"
    "   45: OR         r29, r26, r28\n"
    "   46: GET_ALIAS  r30, a6\n"
    "   47: SLLI       r31, r30, 9\n"
    "   48: OR         r32, r29, r31\n"
    "   49: GET_ALIAS  r33, a7\n"
    "   50: SLLI       r34, r33, 8\n"
    "   51: OR         r35, r32, r34\n"
    "   52: GET_ALIAS  r36, a8\n"
    "   53: SLLI       r37, r36, 3\n"
    "   54: OR         r38, r35, r37\n"
    "   55: GET_ALIAS  r39, a9\n"
    "   56: SLLI       r40, r39, 2\n"
    "   57: OR         r41, r38, r40\n"
    "   58: GET_ALIAS  r42, a10\n"
    "   59: SLLI       r43, r42, 1\n"
    "   60: OR         r44, r41, r43\n"
    "   61: GET_ALIAS  r45, a11\n"
    "   62: OR         r46, r44, r45\n"
    "   63: SET_ALIAS  a3, r46\n"
    "   64: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: int32, no bound storage\n"
    "Alias 11: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,64] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
