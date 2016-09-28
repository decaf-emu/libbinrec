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
    "    1: LOAD       r2, 928(r1)\n"
    "    2: BFEXT      r3, r2, 28, 4\n"
    "    3: SET_ALIAS  a2, r3\n"
    "    4: LABEL      L1\n"
    "    5: GET_ALIAS  r4, a2\n"
    "    6: ANDI       r5, r4, 2\n"
    "    7: GOTO_IF_NZ r5, L3\n"
    "    8: LOAD_IMM   r6, 8\n"
    "    9: SET_ALIAS  a1, r6\n"
    "   10: LABEL      L2\n"
    "   11: GOTO       L2\n"
    "   12: LOAD_IMM   r7, 12\n"
    "   13: SET_ALIAS  a1, r7\n"
    "   14: LABEL      L3\n"
    "   15: LOAD_IMM   r8, 16\n"
    "   16: SET_ALIAS  a1, r8\n"
    "   17: LABEL      L4\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,3] --> 1\n"
    "Block 1: 0 --> [4,7] --> 2,5\n"
    "Block 2: 1 --> [8,9] --> 3\n"
    "Block 3: 2,3 --> [10,11] --> 3\n"
    "Block 4: <none> --> [12,13] --> 5\n"
    "Block 5: 4,1 --> [14,16] --> 6\n"
    "Block 6: 5 --> [17,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
