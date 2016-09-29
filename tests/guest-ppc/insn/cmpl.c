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
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a2\n"
    "    4: GET_ALIAS  r7, a3\n"
    "    5: SLTU       r4, r3, r7\n"
    "    6: SGTU       r5, r3, r7\n"
    "    7: SEQ        r6, r3, r7\n"
    "    8: GET_ALIAS  r9, a5\n"
    "    9: BFEXT      r8, r9, 31, 1\n"
    "   10: SLLI       r10, r6, 1\n"
    "   11: OR         r11, r8, r10\n"
    "   12: SLLI       r12, r5, 2\n"
    "   13: OR         r13, r11, r12\n"
    "   14: SLLI       r14, r4, 3\n"
    "   15: OR         r15, r13, r14\n"
    "   16: SET_ALIAS  a4, r15\n"
    "   17: LOAD_IMM   r16, 4\n"
    "   18: SET_ALIAS  a1, r16\n"
    "   19: LABEL      L2\n"
    "   20: LOAD       r17, 928(r1)\n"
    "   21: ANDI       r18, r17, -16\n"
    "   22: GET_ALIAS  r19, a4\n"
    "   23: OR         r20, r18, r19\n"
    "   24: STORE      928(r1), r20\n"
    "   25: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,18] --> 2\n"
    "Block 2: 1 --> [19,25] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
