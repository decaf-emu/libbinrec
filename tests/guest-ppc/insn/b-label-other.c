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
    0x48,0x00,0x00,0x08,  // b 0x8
    0x48,0x00,0x00,0x08,  // b 0xC
    0x4B,0xFF,0xFF,0xFC,  // b 0x4
    0x4B,0xFF,0xFF,0xF4,  // b 0x0
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GOTO       L3\n"
    "    4: LOAD_IMM   r3, 4\n"
    "    5: SET_ALIAS  a1, r3\n"
    "    6: LABEL      L2\n"
    "    7: GOTO       L4\n"
    "    8: LOAD_IMM   r4, 8\n"
    "    9: SET_ALIAS  a1, r4\n"
    "   10: LABEL      L3\n"
    "   11: GOTO       L2\n"
    "   12: LOAD_IMM   r5, 12\n"
    "   13: SET_ALIAS  a1, r5\n"
    "   14: LABEL      L4\n"
    "   15: GOTO       L1\n"
    "   16: LOAD_IMM   r6, 16\n"
    "   17: SET_ALIAS  a1, r6\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0,7 --> [2,3] --> 5\n"
    "Block 2: <none> --> [4,5] --> 3\n"
    "Block 3: 2,5 --> [6,7] --> 7\n"
    "Block 4: <none> --> [8,9] --> 5\n"
    "Block 5: 4,1 --> [10,11] --> 3\n"
    "Block 6: <none> --> [12,13] --> 7\n"
    "Block 7: 6,3 --> [14,15] --> 1\n"
    "Block 8: <none> --> [16,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
