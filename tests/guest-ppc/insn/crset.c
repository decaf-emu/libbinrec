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
    0x4F,0xC0,0x02,0x42,  // crset 30 (creqv 30,0,0)
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 1\n"
    "    4: SET_ALIAS  a2, r3\n"
    "    5: LOAD_IMM   r4, 4\n"
    "    6: SET_ALIAS  a1, r4\n"
    "    7: LABEL      L2\n"
    "    8: GET_ALIAS  r5, a3\n"
    "    9: ANDI       r6, r5, -3\n"
    "   10: GET_ALIAS  r7, a2\n"
    "   11: SLLI       r8, r7, 1\n"
    "   12: OR         r9, r6, r8\n"
    "   13: SET_ALIAS  a3, r9\n"
    "   14: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,6] --> 2\n"
    "Block 2: 1 --> [7,14] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
