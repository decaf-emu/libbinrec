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
    0x40,0x02,0x00,0x08,  // bc 0,2,0xC
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
    "    5: GET_ALIAS  r4, a3\n"
    "    6: ADDI       r5, r4, -1\n"
    "    7: GOTO_IF_Z  r5, L5\n"
    "    8: GET_ALIAS  r6, a2\n"
    "    9: ANDI       r7, r6, 2\n"
    "   10: SET_ALIAS  a3, r5\n"
    "   11: GOTO_IF_Z  r7, L3\n"
    "   12: LABEL      L5\n"
    "   13: SET_ALIAS  a3, r5\n"
    "   14: LOAD_IMM   r8, 8\n"
    "   15: SET_ALIAS  a1, r8\n"
    "   16: LABEL      L2\n"
    "   17: GOTO       L2\n"
    "   18: LOAD_IMM   r9, 12\n"
    "   19: SET_ALIAS  a1, r9\n"
    "   20: LABEL      L3\n"
    "   21: LOAD_IMM   r10, 16\n"
    "   22: SET_ALIAS  a1, r10\n"
    "   23: LABEL      L4\n"
    "   24: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,3] --> 1\n"
    "Block 1: 0 --> [4,7] --> 2,3\n"
    "Block 2: 1 --> [8,11] --> 3,6\n"
    "Block 3: 2,1 --> [12,15] --> 4\n"
    "Block 4: 3,4 --> [16,17] --> 4\n"
    "Block 5: <none> --> [18,19] --> 6\n"
    "Block 6: 5,2 --> [20,22] --> 7\n"
    "Block 7: 6 --> [23,24] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
