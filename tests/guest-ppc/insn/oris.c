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
    0x64,0x03,0x12,0x34,  // oris r3,r0,4660
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a2\n"
    "    3: ORI        r3, r2, 305397760\n"
    "    4: SET_ALIAS  a3, r3\n"
    "    5: LOAD_IMM   r4, 4\n"
    "    6: SET_ALIAS  a1, r4\n"
    "    7: LABEL      L2\n"
    "    8: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,6] --> 2\n"
    "Block 2: 1 --> [7,8] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
