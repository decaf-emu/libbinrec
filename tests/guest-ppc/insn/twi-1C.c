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
    0x0F,0x83,0x12,0x34,  // twi 28,r3,4660
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 4660\n"
    "    4: GET_ALIAS  r4, a2\n"
    "    5: LOAD_IMM   r5, 0\n"
    "    6: SET_ALIAS  a1, r5\n"
    "    7: LOAD       r6, 968(r1)\n"
    "    8: CALL_ADDR  @r6, r1\n"
    "    9: RETURN\n"
    "   10: LOAD_IMM   r7, 4\n"
    "   11: SET_ALIAS  a1, r7\n"
    "   12: LABEL      L2\n"
    "   13: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,9] --> <none>\n"
    "Block 2: <none> --> [10,11] --> 3\n"
    "Block 3: 2 --> [12,13] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
