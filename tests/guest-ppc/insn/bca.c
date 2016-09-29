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
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 28, 4\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: LABEL      L1\n"
    "    6: LOAD_IMM   r5, -32512\n"
    "    7: GET_ALIAS  r6, a3\n"
    "    8: ADDI       r7, r6, -1\n"
    "    9: GOTO_IF_NZ r7, L3\n"
    "   10: GET_ALIAS  r8, a2\n"
    "   11: ANDI       r9, r8, 2\n"
    "   12: GOTO_IF_Z  r9, L3\n"
    "   13: SET_ALIAS  a3, r7\n"
    "   14: SET_ALIAS  a1, r5\n"
    "   15: GOTO       L2\n"
    "   16: LABEL      L3\n"
    "   17: SET_ALIAS  a3, r7\n"
    "   18: LOAD_IMM   r10, 8\n"
    "   19: SET_ALIAS  a1, r10\n"
    "   20: LABEL      L2\n"
    "   21: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,9] --> 2,4\n"
    "Block 2: 1 --> [10,12] --> 3,4\n"
    "Block 3: 2 --> [13,15] --> 5\n"
    "Block 4: 1,2 --> [16,19] --> 5\n"
    "Block 5: 4,3 --> [20,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
