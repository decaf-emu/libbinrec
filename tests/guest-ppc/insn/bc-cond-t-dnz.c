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
    0x41,0x02,0x00,0x08,  // bc 8,2,0xC
    0x48,0x00,0x00,0x00,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a4\n"
    "    4: ADDI       r4, r3, -1\n"
    "    5: SET_ALIAS  a4, r4\n"
    "    6: GOTO_IF_Z  r4, L5\n"
    "    7: GET_ALIAS  r5, a3\n"
    "    8: ANDI       r6, r5, 536870912\n"
    "    9: GOTO_IF_NZ r6, L3\n"
    "   10: LABEL      L5\n"
    "   11: LOAD_IMM   r7, 8\n"
    "   12: SET_ALIAS  a1, r7\n"
    "   13: LABEL      L2\n"
    "   14: GOTO       L2\n"
    "   15: LOAD_IMM   r8, 12\n"
    "   16: SET_ALIAS  a1, r8\n"
    "   17: LABEL      L3\n"
    "   18: LOAD_IMM   r9, 16\n"
    "   19: SET_ALIAS  a1, r9\n"
    "   20: LABEL      L4\n"
    "   21: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,6] --> 2,3\n"
    "Block 2: 1 --> [7,9] --> 3,6\n"
    "Block 3: 2,1 --> [10,12] --> 4\n"
    "Block 4: 3,4 --> [13,14] --> 4\n"
    "Block 5: <none> --> [15,16] --> 6\n"
    "Block 6: 5,2 --> [17,19] --> 7\n"
    "Block 7: 6 --> [20,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
