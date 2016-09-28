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
    0x7F,0x80,0x04,0x00,  // mcrxr cr7
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: SRLI       r3, r2, 28\n"
    "    4: ANDI       r4, r2, 268435455\n"
    "    5: SET_ALIAS  a2, r3\n"
    "    6: SET_ALIAS  a3, r4\n"
    "    7: LOAD_IMM   r5, 4\n"
    "    8: SET_ALIAS  a1, r5\n"
    "    9: LABEL      L2\n"
    "   10: LOAD       r6, 928(r1)\n"
    "   11: ANDI       r7, r6, -16\n"
    "   12: GET_ALIAS  r8, a2\n"
    "   13: OR         r9, r7, r8\n"
    "   14: STORE      928(r1), r9\n"
    "   15: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,8] --> 2\n"
    "Block 2: 1 --> [9,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
