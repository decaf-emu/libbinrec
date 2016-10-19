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
    0x7C,0x64,0x2F,0xD6,  // divwo r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: GET_ALIAS  r5, a5\n"
    "    5: GOTO_IF_Z  r4, L1\n"
    "    6: SEQI       r6, r3, -2147483648\n"
    "    7: SEQI       r7, r4, -1\n"
    "    8: AND        r8, r6, r7\n"
    "    9: GOTO_IF_NZ r8, L1\n"
    "   10: DIVS       r9, r3, r4\n"
    "   11: SET_ALIAS  a2, r9\n"
    "   12: ANDI       r10, r5, -1073741825\n"
    "   13: SET_ALIAS  a5, r10\n"
    "   14: GOTO       L2\n"
    "   15: LABEL      L1\n"
    "   16: ORI        r11, r5, -1073741824\n"
    "   17: SET_ALIAS  a5, r11\n"
    "   18: LABEL      L2\n"
    "   19: LOAD_IMM   r12, 4\n"
    "   20: SET_ALIAS  a1, r12\n"
    "   21: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,5] --> 1,3\n"
    "Block 1: 0 --> [6,9] --> 2,3\n"
    "Block 2: 1 --> [10,14] --> 4\n"
    "Block 3: 0,1 --> [15,17] --> 4\n"
    "Block 4: 3,2 --> [18,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
