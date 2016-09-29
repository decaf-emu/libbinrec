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
    0x41,0x82,0x00,0x08,  // beq 0xC
    0x48,0x00,0x00,0x00,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 28, 4\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a2\n"
    "    7: ANDI       r6, r5, 2\n"
    "    8: GOTO_IF_NZ r6, L3\n"
    "    9: LOAD_IMM   r7, 8\n"
    "   10: SET_ALIAS  a1, r7\n"
    "   11: LABEL      L2\n"
    "   12: GOTO       L2\n"
    "   13: LOAD_IMM   r8, 12\n"
    "   14: SET_ALIAS  a1, r8\n"
    "   15: LABEL      L3\n"
    "   16: LOAD_IMM   r9, 16\n"
    "   17: SET_ALIAS  a1, r9\n"
    "   18: LABEL      L4\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,8] --> 2,5\n"
    "Block 2: 1 --> [9,10] --> 3\n"
    "Block 3: 2,3 --> [11,12] --> 3\n"
    "Block 4: <none> --> [13,14] --> 5\n"
    "Block 5: 4,1 --> [15,17] --> 6\n"
    "Block 6: 5 --> [18,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
