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
    0x41,0x80,0x00,0x08,  // blt 0xC
    0x48,0x00,0x00,0x00,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    /* This should not trigger an operand assertion on constant range. */
    "    4: ANDI       r4, r3, -2147483648\n"
    "    5: GOTO_IF_NZ r4, L3\n"
    "    6: LOAD_IMM   r5, 8\n"
    "    7: SET_ALIAS  a1, r5\n"
    "    8: LABEL      L2\n"
    "    9: GOTO       L2\n"
    "   10: LOAD_IMM   r6, 12\n"
    "   11: SET_ALIAS  a1, r6\n"
    "   12: LABEL      L3\n"
    "   13: LOAD_IMM   r7, 16\n"
    "   14: SET_ALIAS  a1, r7\n"
    "   15: LABEL      L4\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,5] --> 2,5\n"
    "Block 2: 1 --> [6,7] --> 3\n"
    "Block 3: 2,3 --> [8,9] --> 3\n"
    "Block 4: <none> --> [10,11] --> 5\n"
    "Block 5: 4,1 --> [12,14] --> 6\n"
    "Block 6: 5 --> [15,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
