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
    0x0C,0xA3,0x12,0x34,  // twi 5,r3,4660
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 4660\n"
    "    4: GET_ALIAS  r4, a2\n"
    "    5: SLTU       r5, r4, r3\n"
    "    6: GOTO_IF_NZ r5, L3\n"
    "    7: LOAD_IMM   r6, 0\n"
    "    8: SET_ALIAS  a1, r6\n"
    "    9: LOAD       r7, 984(r1)\n"
    "   10: CALL_ADDR  @r7, r1\n"
    "   11: RETURN\n"
    "   12: LABEL      L3\n"
    "   13: LOAD_IMM   r8, 4\n"
    "   14: SET_ALIAS  a1, r8\n"
    "   15: LABEL      L2\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,6] --> 2,3\n"
    "Block 2: 1 --> [7,11] --> <none>\n"
    "Block 3: 1 --> [12,14] --> 4\n"
    "Block 4: 3 --> [15,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
