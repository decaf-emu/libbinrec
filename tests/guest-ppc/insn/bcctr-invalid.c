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
    0x4E,0x40,0x04,0x20,  // bdzctr (invalid)
};

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: ILLEGAL\n"
    "    4: LOAD_IMM   r3, 4\n"
    "    5: SET_ALIAS  a1, r3\n"
    "    6: LABEL      L2\n"
    "    7: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,5] --> 2\n"
    "Block 2: 1 --> [6,7] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"