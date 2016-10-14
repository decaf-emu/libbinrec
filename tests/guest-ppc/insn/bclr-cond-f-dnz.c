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
    0x4C,0x02,0x00,0x20,  // bclr 0,2
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a4\n"
    "    4: ANDI       r4, r3, -4\n"
    "    5: GET_ALIAS  r5, a5\n"
    "    6: ADDI       r6, r5, -1\n"
    "    7: SET_ALIAS  a5, r6\n"
    "    8: GOTO_IF_Z  r6, L3\n"
    "    9: GET_ALIAS  r7, a3\n"
    "   10: ANDI       r8, r7, 536870912\n"
    "   11: GOTO_IF_NZ r8, L3\n"
    "   12: SET_ALIAS  a1, r4\n"
    "   13: GOTO       L2\n"
    "   14: LABEL      L3\n"
    "   15: LOAD_IMM   r9, 4\n"
    "   16: SET_ALIAS  a1, r9\n"
    "   17: LABEL      L2\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32 @ 932(r1)\n"
    "Alias 5: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,8] --> 2,4\n"
    "Block 2: 1 --> [9,11] --> 3,4\n"
    "Block 3: 2 --> [12,13] --> 5\n"
    "Block 4: 1,2 --> [14,16] --> 5\n"
    "Block 5: 4,3 --> [17,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
