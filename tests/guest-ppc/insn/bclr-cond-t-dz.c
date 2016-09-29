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
    0x4D,0x42,0x00,0x20,  // bclr 10,2
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 28, 4\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a3\n"
    "    7: ANDI       r6, r5, -4\n"
    "    8: GET_ALIAS  r7, a4\n"
    "    9: ADDI       r8, r7, -1\n"
    "   10: GOTO_IF_NZ r8, L3\n"
    "   11: GET_ALIAS  r9, a2\n"
    "   12: ANDI       r10, r9, 2\n"
    "   13: GOTO_IF_Z  r10, L3\n"
    "   14: SET_ALIAS  a4, r8\n"
    "   15: SET_ALIAS  a1, r6\n"
    "   16: GOTO       L2\n"
    "   17: LABEL      L3\n"
    "   18: SET_ALIAS  a4, r8\n"
    "   19: LOAD_IMM   r11, 4\n"
    "   20: SET_ALIAS  a1, r11\n"
    "   21: LABEL      L2\n"
    "   22: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 932(r1)\n"
    "Alias 4: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,10] --> 2,4\n"
    "Block 2: 1 --> [11,13] --> 3,4\n"
    "Block 3: 2 --> [14,16] --> 5\n"
    "Block 4: 1,2 --> [17,20] --> 5\n"
    "Block 5: 4,3 --> [21,22] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
