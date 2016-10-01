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
    0x84,0x64,0xFF,0xF0,  // lwzu r3,-16(r4)
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: ADDI       r4, r3, -16\n"
    "    5: ZCAST      r5, r4\n"
    "    6: ADD        r6, r2, r5\n"
    "    7: LOAD_BR    r7, 0(r6)\n"
    "    8: SET_ALIAS  a2, r7\n"
    "    9: SET_ALIAS  a3, r4\n"
    "   10: LOAD_IMM   r8, 4\n"
    "   11: SET_ALIAS  a1, r8\n"
    "   12: LABEL      L2\n"
    "   13: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,11] --> 2\n"
    "Block 2: 1 --> [12,13] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
