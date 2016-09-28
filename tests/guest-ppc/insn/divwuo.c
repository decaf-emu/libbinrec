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
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: GET_ALIAS  r3, a4\n"
    "    4: GET_ALIAS  r4, a5\n"
    "    5: GOTO_IF_Z  r3, L3\n"
    "    6: DIVU       r5, r2, r3\n"
    "    7: ANDI       r6, r4, -1073741825\n"
    "    8: SET_ALIAS  a5, r6\n"
    "    9: GOTO       L4\n"
    "   10: LABEL      L3\n"
    "   11: ORI        r7, r4, -1073741824\n"
    "   12: SET_ALIAS  a5, r7\n"
    "   13: LABEL      L4\n"
    "   14: SET_ALIAS  a2, r5\n"
    "   15: LOAD_IMM   r8, 4\n"
    "   16: SET_ALIAS  a1, r8\n"
    "   17: LABEL      L2\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,5] --> 2,3\n"
    "Block 2: 1 --> [6,9] --> 4\n"
    "Block 3: 1 --> [10,12] --> 4\n"
    "Block 4: 3,2 --> [13,16] --> 5\n"
    "Block 5: 4 --> [17,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
