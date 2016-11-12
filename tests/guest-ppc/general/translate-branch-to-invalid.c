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

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GOTO       L2\n"
    "    3: LOAD_IMM   r3, 4\n"
    "    4: SET_ALIAS  a1, r3\n"
    "    5: LABEL      L1\n"
    "    6: LOAD_IMM   r4, 8\n"
    "    7: SET_ALIAS  a1, r4\n"
    "    8: GOTO       L3\n"
    "    9: LABEL      L2\n"
    "   10: GOTO       L1\n"
    "   11: LOAD_IMM   r5, 16\n"
    "   12: SET_ALIAS  a1, r5\n"
    "   13: LABEL      L3\n"
    "   14: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,2] --> 3\n"
    "Block 1: <none> --> [3,4] --> 2\n"
    "Block 2: 1,3 --> [5,8] --> 5\n"
    "Block 3: 0 --> [9,10] --> 2\n"
    "Block 4: <none> --> [11,12] --> 5\n"
    "Block 5: 4,2 --> [13,14] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
