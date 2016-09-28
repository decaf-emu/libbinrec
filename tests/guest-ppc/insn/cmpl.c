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
    0x7F,0x83,0x20,0x40,  // cmpl cr7,r3,r4
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a2\n"
    "    3: GET_ALIAS  r6, a3\n"
    "    4: SLTU       r3, r2, r6\n"
    "    5: SGTU       r4, r2, r6\n"
    "    6: SEQ        r5, r2, r6\n"
    "    7: GET_ALIAS  r8, a5\n"
    "    8: BFEXT      r7, r8, 31, 1\n"
    "    9: SLLI       r9, r5, 1\n"
    "   10: OR         r10, r7, r9\n"
    "   11: SLLI       r11, r4, 2\n"
    "   12: OR         r12, r10, r11\n"
    "   13: SLLI       r13, r3, 3\n"
    "   14: OR         r14, r12, r13\n"
    "   15: SET_ALIAS  a4, r14\n"
    "   16: LOAD_IMM   r15, 4\n"
    "   17: SET_ALIAS  a1, r15\n"
    "   18: LABEL      L2\n"
    "   19: LOAD       r16, 928(r1)\n"
    "   20: ANDI       r17, r16, -16\n"
    "   21: GET_ALIAS  r18, a4\n"
    "   22: OR         r19, r17, r18\n"
    "   23: STORE      928(r1), r19\n"
    "   24: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,17] --> 2\n"
    "Block 2: 1 --> [18,24] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
