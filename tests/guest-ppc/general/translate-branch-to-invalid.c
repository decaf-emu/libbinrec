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
    0x48,0x00,0x00,0x0C,  // b 0xC
    0x00,0x00,0x00,0x00,  // (invalid)
    0x00,0x00,0x00,0x00,  // (invalid)
    0x4B,0xFF,0xFF,0xFC,  // b 0x8
};

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GOTO       L3\n"
    "    4: LOAD_IMM   r3, 4\n"
    "    5: SET_ALIAS  a1, r3\n"
    "    6: LABEL      L2\n"
    "    7: LOAD_IMM   r4, 8\n"
    "    8: SET_ALIAS  a1, r4\n"
    "    9: GOTO       L4\n"
    "   10: LABEL      L3\n"
    "   11: GOTO       L2\n"
    "   12: LOAD_IMM   r5, 16\n"
    "   13: SET_ALIAS  a1, r5\n"
    "   14: LABEL      L4\n"
    "   15: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,3] --> 4\n"
    "Block 2: <none> --> [4,5] --> 3\n"
    "Block 3: 2,4 --> [6,9] --> 6\n"
    "Block 4: 1 --> [10,11] --> 3\n"
    "Block 5: <none> --> [12,13] --> 6\n"
    "Block 6: 5,3 --> [14,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
