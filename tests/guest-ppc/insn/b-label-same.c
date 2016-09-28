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
    0x60,0x00,0x00,0x00,  // nop
    0x48,0x00,0x00,0x00,  // b 0x4
};

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: LOAD_IMM   r2, 4\n"
    "    3: SET_ALIAS  a1, r2\n"
    "    4: LABEL      L2\n"
    "    5: GOTO       L2\n"
    "    6: LOAD_IMM   r3, 8\n"
    "    7: SET_ALIAS  a1, r3\n"
    "    8: LABEL      L3\n"
    "    9: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,3] --> 2\n"
    "Block 2: 1,2 --> [4,5] --> 2\n"
    "Block 3: <none> --> [6,7] --> 4\n"
    "Block 4: 3 --> [8,9] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
