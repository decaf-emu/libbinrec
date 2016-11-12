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
    0x42,0x02,0x00,0x08,  // bdnz 0xC
    0x48,0x00,0x00,0x00,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ADDI       r4, r3, -1\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: GOTO_IF_NZ r4, L2\n"
    "    6: LOAD_IMM   r5, 8\n"
    "    7: SET_ALIAS  a1, r5\n"
    "    8: LABEL      L1\n"
    "    9: GOTO       L1\n"
    "   10: LOAD_IMM   r6, 12\n"
    "   11: SET_ALIAS  a1, r6\n"
    "   12: LABEL      L2\n"
    "   13: LOAD_IMM   r7, 16\n"
    "   14: SET_ALIAS  a1, r7\n"
    "   15: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,5] --> 1,4\n"
    "Block 1: 0 --> [6,7] --> 2\n"
    "Block 2: 1,2 --> [8,9] --> 2\n"
    "Block 3: <none> --> [10,11] --> 4\n"
    "Block 4: 3,0 --> [12,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
