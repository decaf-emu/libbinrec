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
    0x38,0x60,0x00,0x00,  // li r3,0
    0x7C,0x64,0x2B,0xD6,  // divw r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 0\n"
    "    4: SET_ALIAS  a2, r3\n"
    "    5: GET_ALIAS  r4, a3\n"
    "    6: GET_ALIAS  r5, a4\n"
    "    7: GOTO_IF_Z  r5, L3\n"
    "    8: SEQI       r6, r4, -2147483648\n"
    "    9: SEQI       r7, r5, -1\n"
    "   10: AND        r8, r6, r7\n"
    "   11: GOTO_IF_NZ r8, L3\n"
    "   12: DIVS       r9, r4, r5\n"
    "   13: SET_ALIAS  a2, r9\n"
    "   14: LABEL      L3\n"
    "   15: LOAD_IMM   r10, 8\n"
    "   16: SET_ALIAS  a1, r10\n"
    "   17: LABEL      L2\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,7] --> 2,4\n"
    "Block 2: 1 --> [8,11] --> 3,4\n"
    "Block 3: 2 --> [12,13] --> 4\n"
    "Block 4: 3,1,2 --> [14,16] --> 5\n"
    "Block 5: 4 --> [17,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
