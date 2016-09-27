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
    0x4E,0x40,0x00,0x20,  // bdzlr
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a2\n"
    "    3: ANDI       r3, r2, -4\n"
    "    4: GET_ALIAS  r4, a3\n"
    "    5: ADDI       r5, r4, -1\n"
    "    6: GOTO_IF_NZ r5, L3\n"
    "    7: SET_ALIAS  a3, r5\n"
    "    8: SET_ALIAS  a1, r3\n"
    "    9: GOTO       L2\n"
    "   10: LABEL      L3\n"
    "   11: SET_ALIAS  a3, r5\n"
    "   12: LOAD_IMM   r6, 4\n"
    "   13: SET_ALIAS  a1, r6\n"
    "   14: LABEL      L2\n"
    "   15: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 932(r1)\n"
    "Alias 3: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,6] --> 2,3\n"
    "Block 2: 1 --> [7,9] --> 4\n"
    "Block 3: 1 --> [10,13] --> 4\n"
    "Block 4: 3,2 --> [14,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
