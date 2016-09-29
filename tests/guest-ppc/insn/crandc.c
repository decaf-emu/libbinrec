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
    0x4F,0xD5,0x61,0x02,  // crandc 30,21,12
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
    "   14: XORI       r11, r10, 1\n"
    "   15: AND        r12, r8, r11\n"
    "   16: GET_ALIAS  r13, a4\n"
    "   17: BFINS      r14, r13, r12, 1, 1\n"
    "   18: SET_ALIAS  a4, r14\n"
    "   19: LOAD_IMM   r15, 4\n"
    "   20: SET_ALIAS  a1, r15\n"
    "   21: LABEL      L2\n"
    "   22: LOAD       r16, 928(r1)\n"
    "   23: ANDI       r17, r16, -16\n"
    "   24: GET_ALIAS  r18, a4\n"
    "   25: OR         r19, r17, r18\n"
    "   26: STORE      928(r1), r19\n"
    "   27: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1\n"
    "Block 1: 0 --> [9,20] --> 2\n"
    "Block 2: 1 --> [21,27] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
