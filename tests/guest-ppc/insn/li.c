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
    0x38,0x60,0x12,0x34,  // li r3,4660
    0x38,0x80,0xAB,0xCD,  // li r4,-21555
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 4660\n"
    "    4: LOAD_IMM   r4, -21555\n"
    "    5: SET_ALIAS  a2, r3\n"
    "    6: SET_ALIAS  a3, r4\n"
    "    7: LOAD_IMM   r5, 8\n"
    "    8: SET_ALIAS  a1, r5\n"
    "    9: LABEL      L2\n"
    "   10: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,8] --> 2\n"
    "Block 2: 1 --> [9,10] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
