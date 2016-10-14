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
    0x41,0x42,0x81,0x02,  // bca 10,2,0xFFFF8100
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, -32512\n"
    "    4: GET_ALIAS  r4, a4\n"
    "    5: ADDI       r5, r4, -1\n"
    "    6: SET_ALIAS  a4, r5\n"
    "    7: GOTO_IF_NZ r5, L3\n"
    "    8: GET_ALIAS  r6, a3\n"
    "    9: ANDI       r7, r6, 536870912\n"
    "   10: GOTO_IF_Z  r7, L3\n"
    "   11: SET_ALIAS  a1, r3\n"
    "   12: GOTO       L2\n"
    "   13: LABEL      L3\n"
    "   14: LOAD_IMM   r8, 8\n"
    "   15: SET_ALIAS  a1, r8\n"
    "   16: LABEL      L2\n"
    "   17: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,7] --> 2,4\n"
    "Block 2: 1 --> [8,10] --> 3,4\n"
    "Block 3: 2 --> [11,12] --> 5\n"
    "Block 4: 1,2 --> [13,15] --> 5\n"
    "Block 5: 4,3 --> [16,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
