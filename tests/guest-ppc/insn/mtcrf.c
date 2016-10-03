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
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 8, 4\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: BFEXT      r5, r3, 0, 4\n"
    "    6: SET_ALIAS  a4, r5\n"
    "    7: LABEL      L1\n"
    "    8: GET_ALIAS  r6, a2\n"
    "    9: BFEXT      r7, r6, 8, 4\n"
    "   10: BFEXT      r8, r6, 0, 4\n"
    "   11: SET_ALIAS  a3, r7\n"
    "   12: SET_ALIAS  a4, r8\n"
    "   13: LOAD_IMM   r9, 4\n"
    "   14: SET_ALIAS  a1, r9\n"
    "   15: LABEL      L2\n"
    "   16: LOAD       r10, 928(r1)\n"
    "   17: ANDI       r11, r10, -3856\n"
    "   18: GET_ALIAS  r12, a3\n"
    "   19: SLLI       r13, r12, 8\n"
    "   20: OR         r14, r11, r13\n"
    "   21: GET_ALIAS  r15, a4\n"
    "   22: OR         r16, r14, r15\n"
    "   23: STORE      928(r1), r16\n"
    "   24: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 1\n"
    "Block 1: 0 --> [7,14] --> 2\n"
    "Block 2: 1 --> [15,24] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
