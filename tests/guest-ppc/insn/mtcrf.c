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
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a2\n"
    "    3: BFEXT      r3, r2, 8, 4\n"
    "    4: SET_ALIAS  a3, r3\n"
    "    5: BFEXT      r4, r2, 0, 4\n"
    "    6: SET_ALIAS  a4, r4\n"
    "    7: LOAD_IMM   r5, 4\n"
    "    8: SET_ALIAS  a1, r5\n"
    "    9: LABEL      L2\n"
    "   10: LOAD       r6, 928(r1)\n"
    "   11: ANDI       r7, r6, -3856\n"
    "   12: GET_ALIAS  r8, a3\n"
    "   13: SLLI       r9, r8, 8\n"
    "   14: OR         r10, r7, r9\n"
    "   15: GET_ALIAS  r11, a4\n"
    "   16: OR         r12, r10, r11\n"
    "   17: STORE      928(r1), r12\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,8] --> 2\n"
    "Block 2: 1 --> [9,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
