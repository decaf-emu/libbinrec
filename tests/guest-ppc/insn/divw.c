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
    0x7C,0x64,0x2B,0xD6,  // divw r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: GET_ALIAS  r4, a4\n"
    "    5: GOTO_IF_Z  r4, L3\n"
    "    6: SEQI       r5, r3, -2147483648\n"
    "    7: SEQI       r6, r4, -1\n"
    "    8: AND        r7, r5, r6\n"
    "    9: GOTO_IF_NZ r7, L3\n"
    "   10: DIVS       r8, r3, r4\n"
    "   11: LABEL      L3\n"
    "   12: SET_ALIAS  a2, r8\n"
    "   13: LOAD_IMM   r9, 4\n"
    "   14: SET_ALIAS  a1, r9\n"
    "   15: LABEL      L2\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,5] --> 2,4\n"
    "Block 2: 1 --> [6,9] --> 3,4\n"
    "Block 3: 2 --> [10,10] --> 4\n"
    "Block 4: 3,1,2 --> [11,14] --> 5\n"
    "Block 5: 4 --> [15,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
