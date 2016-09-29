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
    0x7C,0x64,0x2F,0x96,  // divwuo r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: GET_ALIAS  r4, a4\n"
    "    5: GET_ALIAS  r5, a5\n"
    "    6: GOTO_IF_Z  r4, L3\n"
    "    7: DIVU       r6, r3, r4\n"
    "    8: ANDI       r7, r5, -1073741825\n"
    "    9: SET_ALIAS  a5, r7\n"
    "   10: GOTO       L4\n"
    "   11: LABEL      L3\n"
    "   12: ORI        r8, r5, -1073741824\n"
    "   13: SET_ALIAS  a5, r8\n"
    "   14: LABEL      L4\n"
    "   15: SET_ALIAS  a2, r6\n"
    "   16: LOAD_IMM   r9, 4\n"
    "   17: SET_ALIAS  a1, r9\n"
    "   18: LABEL      L2\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,6] --> 2,3\n"
    "Block 2: 1 --> [7,10] --> 4\n"
    "Block 3: 1 --> [11,13] --> 4\n"
    "Block 4: 3,2 --> [14,17] --> 5\n"
    "Block 5: 4 --> [18,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
