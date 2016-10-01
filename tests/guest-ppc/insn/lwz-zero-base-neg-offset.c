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
    0x80,0x60,0xFF,0xF0,  // lwz r3,-16(0)
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 0xFFFFFFF0\n"
    "    4: ADD        r4, r2, r3\n"
    "    5: LOAD_BR    r5, 0(r4)\n"
    "    6: SET_ALIAS  a2, r5\n"
    "    7: LOAD_IMM   r6, 4\n"
    "    8: SET_ALIAS  a1, r6\n"
    "    9: LABEL      L2\n"
    "   10: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,8] --> 2\n"
    "Block 2: 1 --> [9,10] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
