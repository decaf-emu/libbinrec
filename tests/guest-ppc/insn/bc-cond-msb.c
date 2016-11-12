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

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    /* This should not trigger an operand assertion on constant range. */
    "    3: ANDI       r4, r3, -2147483648\n"
    "    4: GOTO_IF_NZ r4, L2\n"
    "    5: LOAD_IMM   r5, 8\n"
    "    6: SET_ALIAS  a1, r5\n"
    "    7: LABEL      L1\n"
    "    8: GOTO       L1\n"
    "    9: LOAD_IMM   r6, 12\n"
    "   10: SET_ALIAS  a1, r6\n"
    "   11: LABEL      L2\n"
    "   12: LOAD_IMM   r7, 16\n"
    "   13: SET_ALIAS  a1, r7\n"
    "   14: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1,4\n"
    "Block 1: 0 --> [5,6] --> 2\n"
    "Block 2: 1,2 --> [7,8] --> 2\n"
    "Block 3: <none> --> [9,10] --> 4\n"
    "Block 4: 3,0 --> [11,14] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
