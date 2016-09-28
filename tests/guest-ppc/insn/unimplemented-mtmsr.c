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
    0x7C,0x60,0x01,0x24,  // mtmsr r3
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "[warning] Unsupported instruction 7C600124 at address 0x0, treating as invalid\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: ILLEGAL\n"
    "    3: LOAD_IMM   r2, 4\n"
    "    4: SET_ALIAS  a1, r2\n"
    "    5: LABEL      L2\n"
    "    6: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,4] --> 2\n"
    "Block 2: 1 --> [5,6] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
