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
    0x4D,0x82,0x00,0x20,  // beqlr
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD       r2, 928(r1)\n"
    "    2: BFEXT      r3, r2, 28, 4\n"
    "    3: SET_ALIAS  a2, r3\n"
    "    4: LABEL      L1\n"
    "    5: GET_ALIAS  r4, a3\n"
    "    6: ANDI       r5, r4, -4\n"
    "    7: GET_ALIAS  r6, a2\n"
    "    8: ANDI       r7, r6, 2\n"
    "    9: GOTO_IF_Z  r7, L3\n"
    "   10: SET_ALIAS  a1, r5\n"
    "   11: GOTO       L2\n"
    "   12: LABEL      L3\n"
    "   13: LOAD_IMM   r8, 4\n"
    "   14: SET_ALIAS  a1, r8\n"
    "   15: LABEL      L2\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 932(r1)\n"
    "\n"
    "Block 0: <none> --> [0,3] --> 1\n"
    "Block 1: 0 --> [4,9] --> 2,3\n"
    "Block 2: 1 --> [10,11] --> 4\n"
    "Block 3: 1 --> [12,14] --> 4\n"
    "Block 4: 3,2 --> [15,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
