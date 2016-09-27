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
    0x0C,0x23,0x12,0x34,  // twi 1,r3,4660
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: LOAD_IMM   r2, 4660\n"
    "    3: GET_ALIAS  r3, a2\n"
    "    4: SGTU       r4, r3, r2\n"
    "    5: GOTO_IF_Z  r4, L3\n"
    "    6: LOAD_IMM   r5, 0\n"
    "    7: SET_ALIAS  a1, r5\n"
    "    8: LOAD       r6, 968(r1)\n"
    "    9: CALL_ADDR  @r6, r1\n"
    "   10: GOTO       L2\n"
    "   11: LABEL      L3\n"
    "   12: LOAD_IMM   r7, 4\n"
    "   13: SET_ALIAS  a1, r7\n"
    "   14: LABEL      L2\n"
    "   15: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,5] --> 2,3\n"
    "Block 2: 1 --> [6,10] --> 4\n"
    "Block 3: 1 --> [11,13] --> 4\n"
    "Block 4: 3,2 --> [14,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
