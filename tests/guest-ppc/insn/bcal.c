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
    0x41,0x42,0x81,0x03,  // bcal 10,2,0xFFFF8100
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD       r2, 928(r1)\n"
    "    2: BFEXT      r3, r2, 28, 4\n"
    "    3: SET_ALIAS  a2, r3\n"
    "    4: LABEL      L1\n"
    "    5: LOAD_IMM   r4, -32512\n"
    "    6: GET_ALIAS  r5, a4\n"
    "    7: ADDI       r6, r5, -1\n"
    "    8: GOTO_IF_NZ r6, L3\n"
    "    9: GET_ALIAS  r7, a2\n"
    "   10: ANDI       r8, r7, 2\n"
    "   11: GOTO_IF_Z  r8, L3\n"
    "   12: LOAD_IMM   r9, 8\n"
    "   13: SET_ALIAS  a3, r9\n"
    "   14: SET_ALIAS  a4, r6\n"
    "   15: SET_ALIAS  a1, r4\n"
    "   16: GOTO       L2\n"
    "   17: LABEL      L3\n"
    "   18: SET_ALIAS  a4, r6\n"
    "   19: LOAD_IMM   r10, 8\n"
    "   20: SET_ALIAS  a1, r10\n"
    "   21: LABEL      L2\n"
    "   22: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 932(r1)\n"
    "Alias 4: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,3] --> 1\n"
    "Block 1: 0 --> [4,8] --> 2,4\n"
    "Block 2: 1 --> [9,11] --> 3,4\n"
    "Block 3: 2 --> [12,16] --> 5\n"
    "Block 4: 1,2 --> [17,20] --> 5\n"
    "Block 5: 4,3 --> [21,22] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
