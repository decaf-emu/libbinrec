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

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 11, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: BFEXT      r5, r3, 10, 1\n"
    "    6: SET_ALIAS  a4, r5\n"
    "    7: BFEXT      r6, r3, 9, 1\n"
    "    8: SET_ALIAS  a5, r6\n"
    "    9: BFEXT      r7, r3, 8, 1\n"
    "   10: SET_ALIAS  a6, r7\n"
    "   11: BFEXT      r8, r3, 3, 1\n"
    "   12: SET_ALIAS  a7, r8\n"
    "   13: BFEXT      r9, r3, 2, 1\n"
    "   14: SET_ALIAS  a8, r9\n"
    "   15: BFEXT      r10, r3, 1, 1\n"
    "   16: SET_ALIAS  a9, r10\n"
    "   17: BFEXT      r11, r3, 0, 1\n"
    "   18: SET_ALIAS  a10, r11\n"
    "   19: LOAD_IMM   r12, 4\n"
    "   20: SET_ALIAS  a1, r12\n"
    "   21: GET_ALIAS  r13, a11\n"
    "   22: ANDI       r14, r13, -3856\n"
    "   23: GET_ALIAS  r15, a3\n"
    "   24: SLLI       r16, r15, 11\n"
    "   25: OR         r17, r14, r16\n"
    "   26: GET_ALIAS  r18, a4\n"
    "   27: SLLI       r19, r18, 10\n"
    "   28: OR         r20, r17, r19\n"
    "   29: GET_ALIAS  r21, a5\n"
    "   30: SLLI       r22, r21, 9\n"
    "   31: OR         r23, r20, r22\n"
    "   32: GET_ALIAS  r24, a6\n"
    "   33: SLLI       r25, r24, 8\n"
    "   34: OR         r26, r23, r25\n"
    "   35: GET_ALIAS  r27, a7\n"
    "   36: SLLI       r28, r27, 3\n"
    "   37: OR         r29, r26, r28\n"
    "   38: GET_ALIAS  r30, a8\n"
    "   39: SLLI       r31, r30, 2\n"
    "   40: OR         r32, r29, r31\n"
    "   41: GET_ALIAS  r33, a9\n"
    "   42: SLLI       r34, r33, 1\n"
    "   43: OR         r35, r32, r34\n"
    "   44: GET_ALIAS  r36, a10\n"
    "   45: OR         r37, r35, r36\n"
    "   46: SET_ALIAS  a11, r37\n"
    "   47: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: int32, no bound storage\n"
    "Alias 11: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,47] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
