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
    0x0E,0x23,0x12,0x34,  // twi 17,r3,4660
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: LOAD_IMM   r2, 4660\n"
    "    3: GET_ALIAS  r3, a2\n"
    "    4: SLTS       r4, r3, r2\n"
    "    5: SGTU       r5, r3, r2\n"
    "    6: OR         r6, r4, r5\n"
    "    7: GOTO_IF_Z  r6, L3\n"
    "    8: LOAD_IMM   r7, 0\n"
    "    9: SET_ALIAS  a1, r7\n"
    "   10: LOAD       r8, 968(r1)\n"
    "   11: CALL_ADDR  @r8, r1\n"
    "   12: RETURN\n"
    "   13: LABEL      L3\n"
    "   14: LOAD_IMM   r9, 4\n"
    "   15: SET_ALIAS  a1, r9\n"
    "   16: LABEL      L2\n"
    "   17: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,7] --> 2,3\n"
    "Block 2: 1 --> [8,12] --> <none>\n"
    "Block 3: 1 --> [13,15] --> 4\n"
    "Block 4: 3 --> [16,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
