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
    0x4F,0x94,0x00,0x00,  // mcrf cr7,cr5
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
    "   11: GET_ALIAS  r8, a2\n"
    "   12: BFEXT      r9, r8, 11, 1\n"
    "   13: BFEXT      r10, r8, 10, 1\n"
    "   14: BFEXT      r11, r8, 9, 1\n"
    "   15: BFEXT      r12, r8, 8, 1\n"
    "   16: SET_ALIAS  a3, r9\n"
    "   17: SET_ALIAS  a4, r10\n"
    "   18: SET_ALIAS  a5, r11\n"
    "   19: SET_ALIAS  a6, r12\n"
    "   20: LOAD_IMM   r13, 4\n"
    "   21: SET_ALIAS  a1, r13\n"
    "   22: GET_ALIAS  r14, a2\n"
    "   23: ANDI       r15, r14, -16\n"
    "   24: GET_ALIAS  r16, a3\n"
    "   25: SLLI       r17, r16, 3\n"
    "   26: OR         r18, r15, r17\n"
    "   27: GET_ALIAS  r19, a4\n"
    "   28: SLLI       r20, r19, 2\n"
    "   29: OR         r21, r18, r20\n"
    "   30: GET_ALIAS  r22, a5\n"
    "   31: SLLI       r23, r22, 1\n"
    "   32: OR         r24, r21, r23\n"
    "   33: GET_ALIAS  r25, a6\n"
    "   34: OR         r26, r24, r25\n"
    "   35: SET_ALIAS  a2, r26\n"
    "   36: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,36] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
