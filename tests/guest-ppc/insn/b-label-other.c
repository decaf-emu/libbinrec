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

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GOTO       L3\n"
    "    3: LOAD_IMM   r2, 4\n"
    "    4: SET_ALIAS  a1, r2\n"
    "    5: LABEL      L2\n"
    "    6: GOTO       L4\n"
    "    7: LOAD_IMM   r3, 8\n"
    "    8: SET_ALIAS  a1, r3\n"
    "    9: LABEL      L3\n"
    "   10: GOTO       L2\n"
    "   11: LOAD_IMM   r4, 12\n"
    "   12: SET_ALIAS  a1, r4\n"
    "   13: LABEL      L4\n"
    "   14: GOTO       L1\n"
    "   15: LOAD_IMM   r5, 16\n"
    "   16: SET_ALIAS  a1, r5\n"
    "   17: LABEL      L5\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0,7 --> [1,2] --> 5\n"
    "Block 2: <none> --> [3,4] --> 3\n"
    "Block 3: 2,5 --> [5,6] --> 7\n"
    "Block 4: <none> --> [7,8] --> 5\n"
    "Block 5: 4,1 --> [9,10] --> 3\n"
    "Block 6: <none> --> [11,12] --> 7\n"
    "Block 7: 6,3 --> [13,14] --> 1\n"
    "Block 8: <none> --> [15,16] --> 9\n"
    "Block 9: 8 --> [17,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
