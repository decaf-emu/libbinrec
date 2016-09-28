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
    0x7C,0x83,0x2A,0x38,  // eqv r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: GET_ALIAS  r3, a4\n"
    "    4: NOT        r4, r3\n"
    "    5: XOR        r5, r2, r4\n"
    "    6: SET_ALIAS  a2, r5\n"
    "    7: LOAD_IMM   r6, 4\n"
    "    8: SET_ALIAS  a1, r6\n"
    "    9: LABEL      L2\n"
    "   10: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,8] --> 2\n"
    "Block 2: 1 --> [9,10] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
