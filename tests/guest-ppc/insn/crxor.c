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
    0x4F,0xD5,0x61,0x82,  // crxor 30,21,12
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 16, 4\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: BFEXT      r5, r3, 8, 4\n"
    "    6: SET_ALIAS  a3, r5\n"
    "    7: BFEXT      r6, r3, 0, 4\n"
    "    8: SET_ALIAS  a4, r6\n"
    "    9: LABEL      L1\n"
    "   10: GET_ALIAS  r7, a3\n"
    "   11: BFEXT      r8, r7, 2, 1\n"
    "   12: GET_ALIAS  r9, a2\n"
    "   13: BFEXT      r10, r9, 3, 1\n"
    "   14: XOR        r11, r8, r10\n"
    "   15: GET_ALIAS  r12, a4\n"
    "   16: BFINS      r13, r12, r11, 1, 1\n"
    "   17: SET_ALIAS  a4, r13\n"
    "   18: LOAD_IMM   r14, 4\n"
    "   19: SET_ALIAS  a1, r14\n"
    "   20: LABEL      L2\n"
    "   21: LOAD       r15, 928(r1)\n"
    "   22: ANDI       r16, r15, -16\n"
    "   23: GET_ALIAS  r17, a4\n"
    "   24: OR         r18, r16, r17\n"
    "   25: STORE      928(r1), r18\n"
    "   26: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1\n"
    "Block 1: 0 --> [9,19] --> 2\n"
    "Block 2: 1 --> [20,26] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
