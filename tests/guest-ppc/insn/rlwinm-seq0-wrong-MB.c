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
    0x7C,0x64,0x00,0x34,  // cntlzw r4,r3
    0x54,0x83,0xD9,0x3E,  // rlwinm r3,r4,27,4,31
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a2\n"
    "    4: CLZ        r4, r3\n"
    "    5: RORI       r5, r4, 5\n"
    "    6: ANDI       r6, r5, 268435455\n"
    "    7: SET_ALIAS  a2, r6\n"
    "    8: SET_ALIAS  a3, r4\n"
    "    9: LOAD_IMM   r7, 8\n"
    "   10: SET_ALIAS  a1, r7\n"
    "   11: LABEL      L2\n"
    "   12: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,10] --> 2\n"
    "Block 2: 1 --> [11,12] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
