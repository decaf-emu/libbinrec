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
    0x7C,0x6C,0x42,0xE6,  // mftb r3
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: LOAD_IMM   r2, 0\n"
    "    3: SET_ALIAS  a2, r2\n"
    "    4: LOAD_IMM   r3, 4\n"
    "    5: SET_ALIAS  a1, r3\n"
    "    6: LABEL      L2\n"
    "    7: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,5] --> 2\n"
    "Block 2: 1 --> [6,7] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
