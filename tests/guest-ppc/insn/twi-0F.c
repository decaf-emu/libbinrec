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
    0x0D,0xE3,0x12,0x34,  // twi 15,r3,4660
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: LOAD_IMM   r2, 4660\n"
    "    3: GET_ALIAS  r3, a2\n"
    "    4: LOAD_IMM   r4, 0\n"
    "    5: SET_ALIAS  a1, r4\n"
    "    6: LOAD       r5, 968(r1)\n"
    "    7: CALL_ADDR  @r5, r1\n"
    "    8: GOTO       L2\n"
    "    9: LOAD_IMM   r6, 4\n"
    "   10: SET_ALIAS  a1, r6\n"
    "   11: LABEL      L2\n"
    "   12: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,8] --> 3\n"
    "Block 2: <none> --> [9,10] --> 3\n"
    "Block 3: 2,1 --> [11,12] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
